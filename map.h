/**
 * map.h - Self-expanding symbol table structure using arrays and hashtables.
 *
 *  Created on: May 28, 2014
 *      Author: asaparov
 */

#ifndef MAP_H_
#define MAP_H_

#include <time.h>
#include <initializer_list>

#define XXH_PRIVATE_API
#include "array.h"
#include "xxhash.h"

#define RESIZE_THRESHOLD 1 / 2
#define RESIZE_THRESHOLD_INVERSE 2 / 1
#define RESIZE_FACTOR 2

#define XXHASH_SEED 0

typedef void*(alloc_keys_func)(size_t, size_t);


namespace core {

/* forward declarations */

template<typename T>
struct hash_set;

template<typename K, typename V>
struct hash_map;

template<typename K, typename V>
struct array_map;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
template<typename K>
inline uint_fast32_t default_hash(const K& key) {
	return (uint_fast32_t) XXH64(&key, sizeof(K), XXHASH_SEED);
}

template<typename K>
inline uint_fast32_t default_hash(const K* keys, unsigned int length) {
	return (uint_fast32_t) XXH64(keys, sizeof(K) * length, XXHASH_SEED);
}

#else
template<typename K>
inline unsigned int default_hash(const K& key) {
	return XXH32(&key, sizeof(K), XXHASH_SEED);
}

template<typename K>
inline unsigned int default_hash(const K* keys, unsigned int length) {
	return XXH32(keys, sizeof(K) * length, XXHASH_SEED);
}
#endif

template<typename KeyMetric, typename ValueMetric>
struct key_value_metric {
	const KeyMetric& key_metric;
	const ValueMetric& value_metric;

	constexpr key_value_metric(const KeyMetric& key_metric, const ValueMetric& value_metric) :
		key_metric(key_metric), value_metric(value_metric) { }
};

template<typename KeyMetric, typename ValueMetric>
inline constexpr key_value_metric<KeyMetric, ValueMetric> make_key_value_metric(
		const KeyMetric& key_metric, const ValueMetric& value_metric) {
	return key_value_metric<KeyMetric, ValueMetric>(key_metric, value_metric);
}

inline constexpr key_value_metric<dummy_metric, dummy_metric> make_key_value_metric() {
	return make_key_value_metric(dummy_metric(), dummy_metric());
}


/**
 * STL-style iterator implementations (useful for range-based for loops).
 */

template<typename T, bool IsConst>
struct hash_set_iterator {
	typedef typename std::conditional<IsConst, const hash_set<T>&, hash_set<T>&>::type container_type;
	typedef typename std::conditional<IsConst, const T&, T&>::type value_type;

	container_type set;
	unsigned int position;

	hash_set_iterator(container_type& set, unsigned int position) : set(set), position(position) { }

	inline bool operator != (const hash_set_iterator<T, IsConst>& other) const {
		return position != other.position;
	}

	inline value_type operator * () {
		return set.keys[position];
	}

	inline const hash_set_iterator<T, IsConst>& operator ++ () {
		do {
			++position;
		} while (position < set.capacity && hasher<T>::is_empty(set.keys[position]));
		return *this;
	}
};

template<typename K, typename V, bool IsConst>
struct hash_map_iterator {
	typedef typename std::conditional<IsConst, const hash_map<K, V>&, hash_map<K, V>&>::type container_type;
	typedef typename std::conditional<IsConst, pair<const K&, const V&>, pair<K&, V&>>::type value_type;

	container_type map;
	unsigned int position;

	hash_map_iterator(container_type& map, unsigned int position) : map(map), position(position) { }

	inline bool operator != (const hash_map_iterator<K, V, IsConst>& other) const {
		return position != other.position;
	}

	inline value_type operator * () {
		return { map.table.keys[position], map.values[position] };
	}

	inline const hash_map_iterator<K, V, IsConst>& operator ++ () {
		do {
			++position;
		} while (position < map.table.capacity && hasher<K>::is_empty(map.table.keys[position]));
		return *this;
	}
};

template<typename K, typename V, bool IsConst>
struct array_map_iterator {
	typedef typename std::conditional<IsConst, const array_map<K, V>&, array_map<K, V>&>::type container_type;
	typedef typename std::conditional<IsConst, pair<const K&, const V&>, pair<K&, V&>>::type value_type;

	container_type map;
	size_t position;

	array_map_iterator(container_type map, size_t position) : map(map), position(position) { }

	inline bool operator != (const array_map_iterator<K, V, IsConst>& other) const {
		return position != other.position;
	}

	inline value_type operator * () {
		return{ map.keys[position], map.values[position] };
	}

	inline const array_map_iterator<K, V, IsConst>& operator ++ () {
		++position;
		return *this;
	}
};

/**
 * Returns true only if probe > start and probe <= end
 * where probe, start, and end are in the group of
 * integers modulo the capacity of this set.
 */
inline bool index_between(unsigned int probe, unsigned int start, unsigned int end) {
	if (end >= start) {
		return (probe > start && probe <= end);
	} else {
		return (probe <= end || probe > start);
	}
}

/* TODO: consider other collision resolution mechanisms */
template<typename T>
struct hash_set
{
	T* keys;
	unsigned int capacity;
	unsigned int size;

	hash_set(unsigned int initial_capacity, alloc_keys_func alloc_keys = calloc) {
		if (!initialize(initial_capacity, alloc_keys)) {
			fprintf(stderr, "hash_set ERROR: Unable to allocate memory.\n");
			exit(EXIT_FAILURE);
		}
	}

	hash_set(const T* set, unsigned int length,
			alloc_keys_func alloc_keys = calloc) :
				hash_set(length * RESIZE_THRESHOLD_INVERSE + 1, alloc_keys)
	{
		for (unsigned int i = 0; i < length; i++)
			insert(set[i]);
	}

	hash_set(const std::initializer_list<T>& list,
			alloc_keys_func alloc_keys = calloc) :
				hash_set(list.size() * RESIZE_THRESHOLD_INVERSE + 1, alloc_keys)
	{
		typename std::initializer_list<T>::iterator i;
		for (i = list.begin(); i != list.end(); i++)
			insert(*i);
	}

	~hash_set() { free(); }

	bool resize(unsigned int new_capacity,
			alloc_keys_func alloc_keys = calloc)
	{
		T* old_keys = keys;

		keys = (T*) alloc_keys(new_capacity, sizeof(T));
		if (keys == NULL) {
			/* revert changes and return error */
			keys = old_keys;
			return false;
		}

		/* iterate through keys and re-hash the elements */
		unsigned int old_capacity = capacity;
		capacity = new_capacity;
		for (unsigned int i = 0; i < old_capacity; i++) {
			if (!hasher<T>::is_empty(old_keys[i]))
				core::move(old_keys[i], keys[next_empty(old_keys[i])]);
		}
		core::free(old_keys);
		return true;
	}

	inline bool check_size(alloc_keys_func alloc_keys = calloc) {
		return check_size(size, alloc_keys);
	}

	inline bool check_size(unsigned int new_size, alloc_keys_func alloc_keys = calloc)
	{
		while (new_size >= capacity * RESIZE_THRESHOLD) {
			if (!resize(RESIZE_FACTOR * capacity, alloc_keys)) {
				fprintf(stderr, "hash_set.put ERROR: Unable to resize hashtable.\n");
				return false;
			}
		}
		return true;
	}

	bool add(const T& element, alloc_keys_func alloc_keys = calloc)
	{
		if (!check_size(size, alloc_keys)) return false;
		insert(element);
		return true;
	}

	bool add_all(const hash_set<T>& elements,
			alloc_keys_func alloc_keys = calloc)
	{
		if (!check_size(size + elements.size, alloc_keys)) return false;
		for (unsigned int i = 0; i < elements.capacity; i++)
			if (!hasher<T>::is_empty(elements.keys[i]))
				insert(elements.keys[i]);
		return true;
	}

	bool add_all(const T* elements, unsigned int count,
			alloc_keys_func alloc_keys = calloc)
	{
		if (!check_size(size + count, alloc_keys)) return false;
		for (unsigned int i = 0; i < count; i++)
			insert(elements[i]);
		return true;
	}

	bool remove(const T& element)
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hash_set.remove WARNING: Specified key is empty.\n");
#endif

		unsigned int hash_value = hasher<T>::hash(element) % capacity;
		unsigned int index = hash_value;
		while (true) {
			if (keys[index] == element) {
				break;
			} else if (hasher<T>::is_empty(keys[index]))
				return false;
			index = (index + 1) % capacity;
		}

		remove_at(index, hash_value);
		return true;
	}

	template<typename V>
	bool remove(const T& element, V* values)
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hash_map.remove WARNING: Specified key is empty.\n");
#endif

		unsigned int hash_value = hasher<T>::hash(element) % capacity;
		unsigned int index = hash_value;
		while (true) {
			if (keys[index] == element) {
				break;
			} else if (hasher<T>::is_empty(keys[index]))
				return false;
			index = (index + 1) % capacity;
		}

		remove_at(values, index, hash_value);
		return true;
	}

	void remove_at(unsigned int index, unsigned int hash_value)
	{
		unsigned int last = index;
		unsigned int search = (index + 1) % capacity;
		while (!hasher<T>::is_empty(keys[search]))
		{
			unsigned int search_hash = hasher<T>::hash(keys[search]) % capacity;
			if (!index_between(search_hash, last, search)) {
				core::move(keys[search], keys[last]);
				last = search;
			}
			search = (search + 1) % capacity;
		}

		hasher<T>::set_empty(keys[last]);
		size--;
	}

	bool contains(const T& element) const
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hashtable.contains WARNING: Specified key is empty.\n");
		if (size == capacity)
			fprintf(stderr, "hashtable.contains WARNING: Hashtable is full!\n");
#endif

		unsigned int index = hasher<T>::hash(element) % capacity;
		while (true) {
			if (keys[index] == element) {
				return true;
			} else if (hasher<T>::is_empty(keys[index])) {
				return false;
			}
			index = (index + 1) % capacity;
		}
	}

	unsigned int index_of(const T& element) const
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hashtable.index_of WARNING: Specified key is empty.\n");
		if (size == capacity)
			fprintf(stderr, "hashtable.index_of WARNING: Hashtable is full!\n");
#endif

		unsigned int index = hasher<T>::hash(element) % capacity;
		while (keys[index] != element && !hasher<T>::is_empty(keys[index]))
			index = (index + 1) % capacity;
		return index;
	}

	inline unsigned int index_of(
			const T& element, bool& contains) const
	{
		unsigned int hash_value;
		return index_of(element, contains, hash_value);
	}

	unsigned int index_of(const T& element,
			bool& contains, unsigned int& hash_value) const
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hashtable.index_of WARNING: Specified key is empty.\n");
		if (size == capacity)
			fprintf(stderr, "hashtable.index_of WARNING: Hashtable is full!\n");
#endif

		hash_value = hasher<T>::hash(element);
		unsigned int index = hash_value % capacity;
		while (true) {
			if (keys[index] == element) {
				contains = true;
				return index;
			} else if (hasher<T>::is_empty(keys[index])) {
				contains = false;
				return index;
			}
			index = (index + 1) % capacity;
		}
	}

	inline unsigned int index_to_insert(const T& element)
	{
#if !defined(NDEBUG)
		if (size == capacity)
			fprintf(stderr, "hashtable.index_to_insert WARNING: Hashtable is full!\n");
#endif
		unsigned int index = hasher<T>::hash(element) % capacity;
		while (true) {
			if (hasher<T>::is_empty(keys[index])) {
				break;
			} if (keys[index] == element)
				break;
			index = (index + 1) % capacity;
		}
		return index;
	}

	void clear() {
		hasher<T>::set_empty(keys, capacity);
		size = 0;
	}

	bool is_subset(const hash_set<T>& other) const
	{
		for (unsigned int i = 0; i < capacity; i++)
			if (!hasher<T>::is_empty(keys[i]) && !other.contains(keys[i]))
				return false;
		return true;
	}

	bool equals(const hash_set<T>& other) const
	{
		if (size != other.size) return false;
		return is_subset(other);
	}

	/* NOTE: Unlike the libstdc++ unordered_set and unordered_map
	   iterators, we do not keep a linked list among the elements,
	   so if rapid iteration over elements is more critical than
	   rapid queries, consider using an array_map. */
	inline hash_set_iterator<T, false> begin() {
		return hash_set_iterator<T, false>(*this, first_empty());
	}

	inline hash_set_iterator<T, false> end() {
		return hash_set_iterator<T, false>(*this, capacity);
	}

	inline hash_set_iterator<T, true> begin() const {
		return hash_set_iterator<T, true>(*this, first_empty());
	}

	inline hash_set_iterator<T, true> end() const {
		return hash_set_iterator<T, true>(*this, capacity);
	}

	static void swap(hash_set<T>& first, hash_set<T>& second) {
		core::swap(first.keys, second.keys);
		core::swap(first.capacity, second.capacity);
		core::swap(first.size, second.size);
	}

	static inline void move(const hash_set<T>& src, hash_set<T>& dst) {
		dst.keys = src.keys;
		dst.capacity = src.capacity;
		dst.size = src.size;
	}

	static inline bool copy(const hash_set<T>& src, hash_set<T>& dst, alloc_keys_func alloc_keys = calloc) {
		dst.capacity = src.capacity;
		dst.size = src.size;
		dst.keys = (T*) alloc_keys(src.capacity, sizeof(T));
		if (dst.keys == NULL) return false;

		for (unsigned int i = 0; i < src.capacity; i++) {
			if (is_empty(src.keys[i])) continue;
			if (!copy(src.keys[i], dst.keys[i])) {
				free(dst); return false;
			}
		}
		return true;
	}

	template<typename Metric>
	static inline long unsigned int size_of(const hash_set<T>& set, const Metric& metric)
	{
		long unsigned int sum = core::size_of(set.capacity) + core::size_of(set.size);
		for (unsigned int i = 0; i < set.capacity; i++) {
			if (is_empty(set.keys[i]))
				sum += sizeof(T);
			else sum += core::size_of(set.keys[i], metric);
		}
		return sum;
	}

	static inline void free(hash_set<T>& set) { set.free(); }

private:
	bool initialize(
			unsigned int initial_capacity,
			alloc_keys_func alloc_keys)
	{
#if !defined(NDEBUG)
		if (initial_capacity == 0)
			fprintf(stderr, "hashtable.initialize WARNING: Initial capacity is zero.\n");
#endif

		size = 0;
		capacity = initial_capacity;

		keys = (T*) alloc_keys(capacity, sizeof(T));
		return (keys != NULL);
	}

	inline void free() {
		core::free(keys);
	}

	inline void place(
			const T& element, unsigned int index)
	{
#if !defined(NDEBUG)
		if (is_empty(element))
			fprintf(stderr, "hashtable.place WARNING: Specified key is empty.\n");
#endif

		keys[index] = element;
	}

	inline void insert_unique(const T& element)
	{
		place(element, next_empty(element));
	}

	inline unsigned int first_empty() const {
		unsigned int index = 0;
		while (index < capacity && hasher<T>::is_empty(keys[index]))
			index++;
		return index;
	}

	inline unsigned int next_empty(const T& element)
	{
#if !defined(NDEBUG)
		if (size == capacity)
			fprintf(stderr, "hashtable.next_empty WARNING: Hashtable is full!\n");
#endif
		unsigned int index = hasher<T>::hash(element) % capacity;
		while (!hasher<T>::is_empty(keys[index]))
			index = (index + 1) % capacity;
		return index;
	}

	inline void insert(const T& element)
	{
		place(element, index_to_insert(element));
		size++;
	}

	template<typename V>
	void remove_at(
			V* values, unsigned int index,
			unsigned int hash_value)
	{
		unsigned int last = index;
		unsigned int search = (index + 1) % capacity;
		if (!hasher<T>::is_empty(keys[search]))
		{
			do {
				unsigned int search_hash = hasher<T>::hash(keys[search]) % capacity;
				if (!index_between(search_hash, last, search)) {
					core::move(keys[search], keys[last]);
					core::move(values[search], values[last]);
					last = search;
				}
				search = (search + 1) % capacity;
			} while (!hasher<T>::is_empty(keys[search]));
		}

		hasher<T>::set_empty(keys[last]);
		size--;
	}

	template<typename K, typename V>
	friend struct hash_map;

	template<typename K>
	friend bool hash_set_init(hash_set<K>& set,
			unsigned int capacity, alloc_keys_func alloc_keys);

	template<typename K>
	friend bool hash_set_init(hash_set<K>& set,
			const std::initializer_list<T>& list,
			alloc_keys_func alloc_keys);
};

template<typename T>
bool hash_set_init(
		hash_set<T>& set, unsigned int capacity,
		alloc_keys_func alloc_keys = calloc)
{
	if (!set.initialize(capacity, alloc_keys)) {
		fprintf(stderr, "hash_set_init ERROR: Unable to allocate memory.\n");
		return false;
	}
	return true;
}

/* swaps the underlying buffers */
template<typename T>
void swap(hash_set<T>& first, hash_set<T>& second) {
	T* keys_swap = first.keys;
	first.keys = second.keys;
	second.keys = keys_swap;

	unsigned int ui_swap = first.size;
	first.size = second.size;
	second.size = ui_swap;

	ui_swap = first.capacity;
	first.capacity = second.capacity;
	second.capacity = ui_swap;
}

/* TODO: consider other collision resolution mechanisms */
template<typename K, typename V>
struct hash_map
{
	typedef K key_type;
	typedef V value_type;

	hash_set<K> table;
	V* values;

	hash_map(unsigned int capacity, alloc_keys_func alloc_keys = calloc) :
		table(capacity, alloc_keys)
	{
		if (!initialize_values()) {
			fprintf(stderr, "hash_map ERROR: Unable to allocate memory.\n");
			exit(EXIT_FAILURE);
		}
	}

	/**
	 * Specialized constructor where V is an integer type.
	 */
	hash_map(const K* map, unsigned int length,
			alloc_keys_func alloc_keys = calloc) :
				table(length * RESIZE_THRESHOLD_INVERSE + 1, alloc_keys)
	{
		if (!initialize_values()) {
			fprintf(stderr, "hash_map ERROR: Unable to allocate memory.\n");
			exit(EXIT_FAILURE);
		}
		for (unsigned int i = 0; i < length; i++)
			insert(map[i], i);
	}

	hash_map(const K* keys, const V* values,
			unsigned int length, alloc_keys_func alloc_keys = calloc) :
				table(length * RESIZE_THRESHOLD_INVERSE + 1, alloc_keys)
	{
		if (!initialize_values()) {
			fprintf(stderr, "hash_map ERROR: Unable to allocate memory.\n");
			exit(EXIT_FAILURE);
		}
		for (unsigned int i = 0; i < length; i++)
			insert(keys[i], values[i]);
	}

	hash_map(const std::initializer_list<pair<K, V>>& list,
			alloc_keys_func alloc_keys = calloc) :
		table(list.size() * RESIZE_THRESHOLD_INVERSE + 1, alloc_keys)
	{
		if (!initialize_values()) {
			fprintf(stderr, "hash_map ERROR: Unable to allocate memory.\n");
			exit(EXIT_FAILURE);
		}
		typename std::initializer_list<pair<K, V>>::iterator i;
		for (i = list.begin(); i != list.end(); i++)
			insert(i->key, i->value);
	}

	~hash_map() {
		::free(values);
	}

	bool resize(unsigned int new_capacity,
			alloc_keys_func alloc_keys = calloc)
	{
		K* old_keys = table.keys;
		V* old_values = values;

		table.keys = (K*) alloc_keys(new_capacity, sizeof(K));
		if (table.keys == NULL) {
			/* revert changes and return error */
			table.keys = old_keys;
			return false;
		}

		values = (V*) malloc(sizeof(V) * new_capacity);
		if (values == NULL) {
			/* revert changes and return error */
			::free(table.keys);
			table.keys = old_keys;
			values = old_values;
			return false;
		}

		/* iterate through keys and re-hash the elements */
		unsigned int old_capacity = table.capacity;
		table.capacity = new_capacity;
		for (unsigned int i = 0; i < old_capacity; i++) {
			if (!hasher<K>::is_empty(old_keys[i])) {
				unsigned int new_bucket = table.next_empty(old_keys[i]);
				core::move(old_keys[i], table.keys[new_bucket]);
				core::move(old_values[i], values[new_bucket]);
			}
		}
		::free(old_keys);
		::free(old_values);
		return true;
	}

	inline bool check_size(alloc_keys_func alloc_keys = calloc) {
		return check_size(table.size, alloc_keys);
	}

	inline bool check_size(unsigned int new_size, alloc_keys_func alloc_keys = calloc)
	{
		while (new_size >= table.capacity * RESIZE_THRESHOLD) {
			if (!resize(RESIZE_FACTOR * table.capacity, alloc_keys)) {
				fprintf(stderr, "hash_map.put ERROR: Unable to resize hashtable.\n");
				return false;
			}
		}
		return true;
	}

	bool put(const K& key, const V& value,
			alloc_keys_func alloc_keys = calloc)
	{
		if (!check_size(table.size, alloc_keys)) return false;
		insert(key, value);
		return true;
	}

	bool put_all(const hash_map<K, V>& elements,
			alloc_keys_func alloc_keys = calloc)
	{
		if (!check_size(table.size + elements.table.size, alloc_keys))
			return false;
		for (unsigned int i = 0; i < elements.table.capacity; i++)
			if (!hasher<K>::is_empty(elements.table.keys[i]))
				insert(elements.table.keys[i], elements.values[i]);
		return true;
	}

	inline bool remove(const K& key)
	{
		return table.remove(key, values);
	}

	inline void remove_at(unsigned int index, unsigned int hash_value)
	{
		table.remove_at(values, index, hash_value);
	}

	V& get(const K& key) const
	{
		return values[table.index_of(key)];
	}

	inline V& get(const K& key, bool& contains) const
	{
		unsigned int index;
		return get(key, contains, index);
	}

	V& get(const K& key, bool& contains, unsigned int& index) const
	{
		index = table.index_of(key, contains);
		return values[index];
	}

	V& get(const K& key, bool& contains,
			unsigned int& index,
			unsigned int& hash_value) const
	{
		index = table.index_of(key, contains, hash_value);
		return values[index];
	}

	inline void clear() {
		table.clear();
	}

	/* NOTE: Unlike the libstdc++ unordered_set and unordered_map
	   iterators, we do not keep a linked list among the elements,
	   so if rapid iteration over elements is more critical than
	   rapid queries, consider using an array_map. */
	inline hash_map_iterator<K, V, false> begin() {
		return hash_map_iterator<K, V, false>(*this, table.first_empty());
	}

	inline hash_map_iterator<K, V, false> end() {
		return hash_map_iterator<K, V, false>(*this, table.capacity);
	}

	inline hash_map_iterator<K, V, true> begin() const {
		return hash_map_iterator<K, V, true>(*this, table.first_empty());
	}

	inline hash_map_iterator<K, V, true> end() const {
		return hash_map_iterator<K, V, true>(*this, table.capacity);
	}

	static inline void swap(hash_map<K, V>& first, hash_map<K, V>& second) {
		hash_set<K>::swap(first.table, second.table);
		core::swap(first.values, second.values);
	}

	static inline void move(const hash_map<K, V>& src, hash_map<K, V>& dst) {
		hash_set<K>::move(src.table, dst.table);
		dst.values = src.values;
	}

	static inline bool copy(const hash_map<K, V>& src, hash_map<K, V>& dst, alloc_keys_func alloc_keys = calloc) {
		dst.table.capacity = src.table.capacity;
		dst.table.size = src.table.size;
		dst.table.keys = (K*) alloc_keys(src.table.capacity, sizeof(K));
		if (dst.table.keys == NULL) return false;
		dst.values = (V*) malloc(sizeof(V) * src.table.capacity);
		if (dst.values == NULL) {
			free(dst.table.keys);
			return false;
		}

		for (unsigned int i = 0; i < src.table.capacity; i++) {
			if (is_empty(src.table.keys[i])) continue;
			if (!core::copy(src.table.keys[i], dst.table.keys[i])) {
				free(dst); return false;
			} else if (!core::copy(src.values[i], dst.values[i])) {
				core::free(dst.table.keys[i]);
				set_empty(dst.table.keys[i]);
				free(dst); return false;
			}
		}
		return true;
	}

	template<typename KeyMetric, typename ValueMetric>
	static inline long unsigned int size_of(const hash_map<K, V>& map,
			const key_value_metric<KeyMetric, ValueMetric>& metric)
	{
		long unsigned int sum = core::size_of(map.table.capacity) + core::size_of(map.table.size);
		for (unsigned int i = 0; i < map.table.capacity; i++) {
			if (is_empty(map.table.keys[i]))
				sum += sizeof(K) + sizeof(V);
			else sum += core::size_of(map.table.keys[i], metric.key_metric) + core::size_of(map.values[i], metric.value_metric);
		}
		return sum;
	}

	static inline void free(hash_map<K, V>& map) {
		core::free(map.table);
		core::free(map.values);
	}

private:
	/* NOTE: this function assumes table is initialized */
	bool initialize_values() {
		values = (V*) malloc(sizeof(V) * table.capacity);
		if (values == NULL) {
			core::free(table);
			return false;
		}
		return true;
	}

	bool initialize(
			unsigned int initial_capacity,
			alloc_keys_func alloc_keys)
	{
		if (!table.initialize(initial_capacity, alloc_keys))
			return false;
		return initialize_values();
	}

	inline void place(const K& key, const V& value, unsigned int index)
	{
		table.place(key, index);
		values[index] = value;
	}

	inline void insert_unique(
			const K& key, const V& value)
	{
		place(key, value, table.next_empty(key));
	}

	inline void insert(const K& key, const V& value)
	{
		place(key, value, table.index_to_insert(key));
		table.size++;
	}

	template<typename T, typename U>
	friend bool hash_map_init(hash_map<T, U>& map,
			unsigned int capacity, alloc_keys_func alloc_keys);
};

template<typename K, typename V>
bool hash_map_init(
		hash_map<K, V>& map,
		unsigned int capacity,
		alloc_keys_func alloc_keys = calloc)
{
	if (!map.initialize(capacity, alloc_keys)) {
		fprintf(stderr, "hash_map_init ERROR: Unable to allocate memory.\n");
		return false;
	}
	return true;
}

/* swaps the underlying buffers */
template<typename K, typename V>
void swap(hash_map<K, V>& first, hash_map<K, V>& second) {
	K* keys_swap = first.table.keys;
	first.table.keys = second.table.keys;
	second.table.keys = keys_swap;

	V* values_swap = first.values;
	first.values = second.values;
	second.values = values_swap;

	unsigned int ui_swap = first.table.size;
	first.table.size = second.table.size;
	second.table.size = ui_swap;

	ui_swap = first.table.capacity;
	first.table.capacity = second.table.capacity;
	second.table.capacity = ui_swap;
}

template<typename K, typename V>
struct array_map {
	typedef K key_type;
	typedef V value_type;

	K* keys;
	V* values;
	size_t capacity;
	size_t size;

	array_map(unsigned int initial_capacity) : size(0) {
		if (!initialize(initial_capacity)) {
			fprintf(stderr, "array_map ERROR: Error during initialization.\n");
			exit(EXIT_FAILURE);
		}
	}

	~array_map() { free(); }

	bool ensure_capacity(unsigned int new_length) {
		if (new_length <= capacity)
			return true;

		unsigned int new_capacity = (unsigned int) capacity;
		if (!expand(keys, new_capacity, new_length))
			return false;
		if (!resize(values, new_capacity))
			return false;
		capacity = new_capacity;
		return true;
	}

	bool put(const K& key, const V& value) {
		unsigned int index = index_of(key);
		if (index < size) {
			values[index] = value;
			return true;
		}

		if (!ensure_capacity((unsigned int) size + 1))
			return false;
		keys[size] = key;
		values[size] = value;
		size++;
		return true;
	}

	inline unsigned int index_of(const K& key) const {
		return core::index_of(key, keys, size);
	}

	inline unsigned int index_of(const K& key, unsigned int start) const {
		for (unsigned int i = start; i < size; i++)
			if (keys[i] == key)
				return i;
		return (unsigned int) size;
	}

	inline unsigned int last_index_of(const K& key) const {
		return core::last_index_of(key, keys, size);
	}

	inline bool contains(const K& key) const {
		return index_of(key) < size;
	}

	inline V& get(const K& key) {
		return values[index_of(key)];
	}

	inline V& get(const K& key, unsigned int& index) {
		index = index_of(key);
		return values[index];
	}

	inline V& get(const K& key, bool& contains) {
		unsigned int index = index_of(key);
		contains = (index != size);
		return values[index];
	}

	inline const V& get(const K& key, bool& contains) const {
		unsigned int index = index_of(key);
		contains = (index != size);
		return values[index];
	}

	bool remove(const K& key) {
		unsigned int index = index_of(key);
		if (index < size)
			remove_at(index);
		return true;
	}

	inline void remove_at(unsigned int index) {
		size--;
		if (index == size)
			return;

		/* move the last item into the position of the removed item */
		core::move(keys[size], keys[index]);
		core::move(values[size], values[index]);
	}

	inline void clear() {
		size = 0;
	}

	inline array_map_iterator<K, V, false> begin() {
		return array_map_iterator<K, V, false>(*this, 0);
	}

	inline array_map_iterator<K, V, false> end() {
		return array_map_iterator<K, V, false>(*this, size);
	}

	inline array_map_iterator<K, V, true> begin() const {
		return array_map_iterator<K, V, true>(*this, 0);
	}

	inline array_map_iterator<K, V, true> end() const {
		return array_map_iterator<K, V, true>(*this, size);
	}

	static inline void swap(array_map<K, V>& first, array_map<K, V>& second) {
		core::swap(first.keys, second.keys);
		core::swap(first.values, second.values);
		core::swap(first.capacity, second.capacity);
		core::swap(first.size, second.size);
	}

	static inline void move(const array_map<K, V>& src, array_map<K, V>& dst) {
		dst.keys = src.keys;
		dst.values = src.values;
		dst.capacity = src.capacity;
		dst.size = src.size;
	}

	template<typename KeyMetric, typename ValueMetric>
	static inline long unsigned int size_of(const array_map<K, V>& map,
			const key_value_metric<KeyMetric, ValueMetric>& metric)
	{
		long unsigned int sum = core::size_of(map.capacity) + core::size_of(map.size);
		for (unsigned int i = 0; i < map.size; i++)
			sum += core::size_of(map.keys[i], metric.key_metric) + core::size_of(map.values[i], metric.value_metric);
		return sum + (map.capacity - map.size) * (sizeof(K) + sizeof(V));
	}

	static inline long unsigned int size_of(const array_map<K, V>& map, const dummy_metric& metric) {
		return size_of(map, make_key_value_metric(dummy_metric(), dummy_metric()));
	}

	static inline void free(array_map<K, V>& map) { map.free(); }

private:
	inline bool initialize(unsigned int initial_capacity) {
		capacity = initial_capacity;
		keys = (K*) malloc(sizeof(K) * capacity);
		if (keys == NULL) {
			fprintf(stderr, "array_map.initialize ERROR: Out of memory.\n");
			return false;
		}
		values = (V*) malloc(sizeof(V) * capacity);
		if (values == NULL) {
			core::free(keys);
			fprintf(stderr, "array_map.initialize ERROR: Out of memory.\n");
			return false;
		}
		return true;
	}

	inline void free() {
		core::free(keys);
		core::free(values);
	}

	template<typename A, typename B>
	friend bool array_map_init(array_map<A, B>&, unsigned int);
};

template<typename K, typename V>
bool array_map_init(array_map<K, V>& map, unsigned int initial_capacity) {
	map.size = 0;
	return map.initialize(initial_capacity);
}

template<typename T>
inline unsigned int size(const hash_set<T>& set) {
	return set.size;
}

template<typename K, typename V>
inline unsigned int size(const hash_map<K, V>& map) {
	return map.table.size;
}

template<typename K, typename V>
inline unsigned int size(const array_map<K, V>& map) {
	return map.size;
}

template<typename MapType>
inline const typename MapType::key_type** invert(const MapType& map) {
	const typename MapType::key_type** inverse =
			(const typename MapType::key_type**) calloc(size(map) + 1, sizeof(typename MapType::key_type*));
	if (inverse == NULL) {
		fprintf(stderr, "invert ERROR: Unable to invert map. Out of memory.\n");
		return NULL;
	}
	for (const auto& entry : map)
		inverse[entry.value] = &entry.key;
	return inverse;
}

inline bool hash_map_test(void)
{
	hash_map<int, const char*> map = hash_map<int, const char*>(4);
	bool contains_four = true, contains_minus_seven = true;
	map.get(4, contains_four);
	map.get(-7, contains_minus_seven);
	if (contains_four || contains_minus_seven) {
		fprintf(stderr, "hash_map_test ERROR: Map with no inserted elements should be empty.\n");
		return false;
	}

	/* test insertion and retrieval operations */
	map.put(-7, "negative seven");
	map.put(4, "four");

	if (map.table.size != 2) {
		fprintf(stderr, "hash_map_test ERROR: Map size is %d after adding two elements.\n", map.table.size);
		return false;
	}
	if (strcmp(map.get(-7), "negative seven")
	 || strcmp(map.get(4), "four"))
	{
		fprintf(stderr, "hash_map_test ERROR: Simple hashtable insertion failed.\n");
		return false;
	}

	map.put(4, "new four");
	map.put(5, "five");
	map.put(12, "twelve");
	map.put(7, "seven");
	map.put(13, "thirteen");

	/* test automatic capacity resizing */
	if (map.table.size != 6) {
		fprintf(stderr, "hash_map_test ERROR: Map size is %d after adding six elements.\n", map.table.size);
		return false;
	}
	if (map.table.capacity != 4 * RESIZE_FACTOR * RESIZE_FACTOR) {
		fprintf(stderr, "hash_map_test ERROR: Unexpected hashtable capacity.\n");
		return false;
	}
	if (strcmp(map.get(4), "new four")
	 || strcmp(map.get(12), "twelve")
	 || strcmp(map.get(7), "seven")
	 || strcmp(map.get(-7), "negative seven"))
	{
		fprintf(stderr, "hash_map_test ERROR: Additional hashtable insertion failed.\n");
		return false;
	}

	/* test removal operation */
	if (!map.remove(4)) {
		fprintf(stderr, "hash_map_test ERROR: Removal of key '4' failed.\n");
		return false;
	}
	if (strcmp(map.get(12), "twelve")
	 || strcmp(map.get(13), "thirteen")
	 || strcmp(map.get(5), "five")
	 || strcmp(map.get(-7), "negative seven")
	 || strcmp(map.get(7), "seven")) {
		fprintf(stderr, "hash_map_test ERROR: Hashtable lookup failed after removal.\n");
		return false;
	}

	bool contains = true;
	map.get(4, contains);
	if (contains) {
		fprintf(stderr, "hash_map_test ERROR: Retrieval of removed key failed.\n");
		return false;
	}

	return true;
}

inline bool array_map_test(void)
{
	array_map<int, const char*> map = array_map<int, const char*>(4);
	bool contains_four = true, contains_minus_seven = true;
	map.get(4, contains_four);
	map.get(-7, contains_minus_seven);
	if (contains_four || contains_minus_seven) {
		fprintf(stderr, "array_map_test ERROR: Map with no inserted elements should be empty.\n");
		return false;
	}

	/* test insertion and retrieval operations */
	map.put(-7, "negative seven");
	map.put(4, "four");

	if (map.size != 2) {
		fprintf(stderr, "array_map_test ERROR: Map size is %zu after adding two elements.\n", map.size);
		return false;
	}
	if (strcmp(map.get(-7), "negative seven")
	 || strcmp(map.get(4), "four"))
	{
		fprintf(stderr, "array_map_test ERROR: Simple hashtable insertion failed.\n");
		return false;
	}

	map.put(4, "new four");
	map.put(5, "five");
	map.put(12, "twelve");
	map.put(7, "seven");
	map.put(13, "thirteen");

	if (map.size != 6) {
		fprintf(stderr, "array_map_test ERROR: Map size is %zu after adding six elements.\n", map.size);
		return false;
	}
	if (strcmp(map.get(4), "new four")
	 || strcmp(map.get(12), "twelve")
	 || strcmp(map.get(7), "seven")
	 || strcmp(map.get(-7), "negative seven"))
	{
		fprintf(stderr, "array_map_test ERROR: Additional hashtable insertion failed.\n");
		return false;
	}

	/* test removal operation */
	if (!map.remove(4)) {
		fprintf(stderr, "array_map_test ERROR: Removal of key '4' failed.\n");
		return false;
	}
	if (strcmp(map.get(12), "twelve")
	 || strcmp(map.get(13), "thirteen")
	 || strcmp(map.get(5), "five")
	 || strcmp(map.get(-7), "negative seven")
	 || strcmp(map.get(7), "seven")) {
		fprintf(stderr, "array_map_test ERROR: Hashtable lookup failed after removal.\n");
		return false;
	}

	bool contains = true;
	map.get(4, contains);
	if (contains) {
		fprintf(stderr, "array_map_test ERROR: Retrieval of removed key failed.\n");
		return false;
	}

	return true;
}

} /* namespace core */

#endif /* MAP_H_ */
