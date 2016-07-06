/**
 * array.h - Simple structure for an expanding array.
 *
 *  Created on: Mar 3, 2012
 *	  Author: asaparov
 */

#ifndef ARRAY_H_
#define ARRAY_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core.h"

#define RESIZE_FACTOR 2

namespace core {

template<typename T, typename SizeType>
inline bool resize(T*& data, const SizeType& new_capacity) {
	T* new_data = (T*) realloc(data, new_capacity * sizeof(T));
	if (new_data == NULL) {
		fprintf(stderr, "array.resize ERROR: Out of memory.\n");
		return false;
	}
	data = new_data;
	return true;
}

template<typename T, typename SizeType>
inline bool expand(T*& data, SizeType& capacity, size_t new_length) {
	do {
		/* increase the size of the underlying array */
		capacity *= RESIZE_FACTOR;
	} while (new_length > capacity);
	return resize(data, capacity);
}

template<typename T, typename SizeType>
inline bool ensure_capacity(T*& data, SizeType& capacity, size_t new_length)
{
	if (new_length <= capacity)
		return true;
	SizeType new_capacity = capacity;
	if (!expand(data, new_capacity, new_length))
		return false;
	capacity = new_capacity;
	return true;
}

template<typename T>
struct array {
	T* data;
	size_t length;
	size_t capacity;

	array(unsigned int initial_capacity)
	{
		if (!initialize(initial_capacity))
			exit(EXIT_FAILURE);
	}

	~array()
	{
		free();
	}

	inline T& operator[] (unsigned int index) {
		return data[index];
	}

	inline const T& operator[] (unsigned int index) const {
		return data[index];
	}

	inline void clear()
	{
		length = 0;
	}

	void remove(unsigned int index)
	{
		data[index] = data[length - 1];
		length--;
	}

	bool ensure_capacity(size_t new_length) {
		return core::ensure_capacity(data, capacity, new_length);
	}

	bool append(const T* elements, size_t size)
	{
		if (!ensure_capacity(length + size))
			return false;
		memcpy(&data[length], elements, sizeof(T) * size);
		length += size;
		return true;
	}

	inline bool contains(const T& element) const {
		return index_of(element) < length;
	}

	inline unsigned int index_of(const T& element) const {
		for (unsigned int i = 0; i < length; i++)
			if (data[i] == element)
				return i;
		return (unsigned int) length;
	}

	T& first()
	{
		return data[0];
	}

	const T& first() const
	{
		return data[0];
	}

	T& last()
	{
		return data[length - 1];
	}

	const T& last() const
	{
		return data[length - 1];
	}

	bool add(const T& element)
	{
		if (!ensure_capacity(length + 1))
			return false;
		data[length] = element;
		length++;
		return true;
	}

	T pop()
	{
		length--;
		return data[length];
	}

	inline T* begin() {
		return data;
	}

	inline T* end() {
		return data + length;
	}

	inline const T* begin() const {
		return data;
	}

	inline const T* end() const {
		return data + length;
	}

	static inline void move(const array<T>& src, array<T>& dst) {
		dst.length = src.length;
		dst.capacity = src.capacity;
		dst.data = src.data;
	}

	template<typename Metric>
	static inline long unsigned int size_of(const array<T>& a, const Metric& metric) {
		long unsigned int sum = core::size_of(a.capacity) + core::size_of(a.length);
		for (unsigned int i = 0; i < a.length; i++)
			sum += core::size_of(a.data[i], metric);
		return sum + (a.capacity - a.length) * sizeof(T);
	}

private:
	inline bool initialize(unsigned int initial_capacity)
	{
#if !defined(NDEBUG)
		if (initial_capacity == 0)
			fprintf(stderr, "array.initialize WARNING: Initial capacity is zero.\n");
#endif

		capacity = initial_capacity;
		length = 0;
		data = (T*) malloc(sizeof(T) * capacity);
		if (data == NULL) {
			fprintf(stderr, "array.initialize ERROR: Out of memory.\n");
			return false;
		}
		return true;
	}

	inline void free()
	{
		if (data) {
			::free(data);
			data = NULL;
		}
	}

	template<typename K>
	friend bool array_init(array<K>& m, unsigned int initial_capacity);

	template<typename K>
	friend void array_free(array<K>& m);
};

template<typename T>
bool array_init(array<T>& m, unsigned int initial_capacity) {
	return m.initialize(initial_capacity);
}

template<typename T>
void array_free(array<T>& m) {
	m.free();
}

/**
 * Swaps the underlying buffers of the given arrays.
 */
template<typename T>
inline void swap(array<T>& a, array<T>& b)
{
	T* temp = a.data;
	a.data = b.data;
	b.data = temp;

	size_t swap = a.length;
	a.length = b.length;
	b.length = swap;

	swap = a.capacity;
	a.capacity = b.capacity;
	b.capacity = swap;
}

template<typename T, typename Metric>
inline long unsigned int size(const array<T>& a, const Metric& metric) {
	long unsigned int sum = size(a.capacity) + size(a.length);
	for (unsigned int i = 0; i < a.length; i++)
		sum += size(a.data[i], metric);
	return sum + (a.capacity - a.length) * sizeof(T);
}

template<typename T>
inline bool operator == (const array<T>& a, const array<T>& b) {
	if (a.length != b.length)
		return false;
	for (unsigned int i = 0; i < a.length; i++)
		if (a.data[i] != b.data[i])
			return false;
	return true;
}

template<typename T>
inline bool operator != (const array<T>& a, const array<T>& b) {
	if (a.length != b.length)
		return true;
	for (unsigned int i = 0; i < a.length; i++)
		if (a.data[i] != b.data[i])
			return true;
	return false;
}

template<typename T>
void insertion_sort(T* keys, unsigned int count)
{
	for (unsigned int i = 1; i < count; i++) {
		T item = keys[i];
		unsigned int hole = i;

		while (hole > 0 && keys[hole - 1] > item) {
			keys[hole] = keys[hole - 1];
			hole--;
		}

		keys[hole] = item;
	}
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void insertion_sort(T* keys, unsigned int count, const Sorter& sorter)
{
	T& item = *((T*) malloc(sizeof(T)));
	for (unsigned int i = 1; i < count; i++) {
		move(keys[i], item);
		unsigned int hole = i;

		while (hole > 0 && less_than(item, keys[hole - 1], sorter)) {
			move(keys[hole - 1], keys[hole]);
			hole--;
		}

		move(item, keys[hole]);
	}
	free(&item);
}

template<typename K, typename V>
void insertion_sort(K* keys, V* values, unsigned int count)
{
	for (unsigned int i = 1; i < count; i++) {
		K item = keys[i];
		V value = values[i];
		unsigned int hole = i;

		while (hole > 0 && keys[hole - 1] > item) {
			keys[hole] = keys[hole - 1];
			values[hole] = values[hole - 1];
			hole--;
		}

		keys[hole] = item;
		values[hole] = value;
	}
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void insertion_sort(K* keys, V* values, unsigned int count, const Sorter& sorter)
{
	K& item = *((K*) malloc(sizeof(K)));
	V& value = *((V*) malloc(sizeof(V)));
	for (unsigned int i = 1; i < count; i++) {
		move(keys[i], item);
		move(values[i], value);
		unsigned int hole = i;

		while (hole > 0 && less_than(item, keys[hole - 1], sorter)) {
			move(keys[hole - 1], keys[hole]);
			move(values[hole - 1], values[hole]);
			hole--;
		}

		move(item, keys[hole]);
		move(value, values[hole]);
	}
	free(&item); free(&value);
}

template<typename T>
inline void insertion_sort(array<T>& keys) {
	insertion_sort(keys.data, (unsigned int) keys.length);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void insertion_sort(array<T>& keys, const Sorter& sorter)
{
	insertion_sort(keys.data, (unsigned int) keys.length, sorter);
}

template<typename K, typename V>
inline void insertion_sort(array<K>& keys, array<V>& values) {
	insertion_sort(keys.data, values.data, (unsigned int) keys.length);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void insertion_sort(array<K>& keys, array<V>& values, const Sorter& sorter)
{
	insertion_sort(keys.data, values.data, (unsigned int) keys.length, sorter);
}

template<typename T>
void reverse(T* array, unsigned int length) {
	for (unsigned int i = 0; i < length / 2; i++) {
		unsigned int other = length - i - 1;
		swap(array[i], array[other]);
	}
}

template<typename T>
inline void reverse(array<T>& array) {
	reverse(array.data, (unsigned int) array.length);
}

template<typename T>
inline const T& get_pivot(T* array, unsigned int start, unsigned int end) {
	return array[(end + start) / 2];
}

template<typename T>
inline void quick_sort_partition(T* array,
		unsigned int start, unsigned int end,
		unsigned int& l, unsigned int& r)
{
	const T p = get_pivot(array, start, end);
	while (true) {
		while (array[l] < p)
			l++;
		while (p < array[r])
			r--;
		if (l == r) {
			l++;
			if (r > 0) r--;
			return;
		} else if (l > r) return;
		swap(array[l++], array[r--]);
	}
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort_partition(
		T* array, unsigned int start, unsigned int end,
		unsigned int& l, unsigned int& r, const Sorter& sorter)
{
	T& p = *((T*) malloc(sizeof(T)));
	move(get_pivot(array, start, end), p);
	while (true) {
		while (less_than(array[l], p, sorter))
			l++;
		while (less_than(p, array[r], sorter))
			r--;
		if (l == r) {
			l++;
			if (r > 0) r--;
			break;
		} else if (l > r) break;
		swap(array[l++], array[r--]);
	}
	free(&p);
}

template<typename K, typename V>
inline void quick_sort_partition(K* keys, V* values,
		unsigned int start, unsigned int end, unsigned int& l, unsigned int& r)
{
	const K p = get_pivot(keys, start, end);
	while (true) {
		while (keys[l] < p)
			l++;
		while (p < keys[r])
			r--;
		if (l == r) {
			l++;
			if (r > 0) r--;
			return;
		} else if (l > r) return;
		swap(values[l], values[r]);
		swap(keys[l++], keys[r--]);
	}
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort_partition(
		K* keys, V* values, unsigned int start, unsigned int end,
		unsigned int& l, unsigned int& r, const Sorter& sorter)
{
	K& p = *((K*) malloc(sizeof(K)));
	move(get_pivot(keys, start, end), p);
	while(true) {
		while (less_than(keys[l], p, sorter))
			l++;
		while (less_than(p, keys[r], sorter))
			r--;
		if (l == r) {
			l++;
			if (r > 0) r--;
			break;
		} else if (l > r) break;
		swap(values[l], values[r]);
		swap(keys[l++], keys[r--]);
	}
	free(&p);
}

template<typename T>
void quick_sort(T* array, unsigned int start, unsigned int end)
{
	if (start >= end)
		return;
	unsigned int l = start, r = end;
	quick_sort_partition(array, start, end, l, r);
	quick_sort(array, start, r);
	quick_sort(array, l, end);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void quick_sort(T* array, unsigned int start, unsigned int end, const Sorter& sorter)
{
	if (start >= end)
		return;
	unsigned int l = start, r = end;
	quick_sort_partition(array, start, end, l, r, sorter);
	quick_sort(array, start, r, sorter);
	quick_sort(array, l, end, sorter);
}

template<typename K, typename V>
void quick_sort(K* keys, V* values, unsigned int start, unsigned int end)
{
	if (start >= end)
		return;
	unsigned int l = start, r = end;
	quick_sort_partition(keys, values, start, end, l, r);
	quick_sort(keys, values, start, r);
	quick_sort(keys, values, l, end);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void quick_sort(K* keys, V* values,
		unsigned int start, unsigned int end, const Sorter& sorter)
{
	if (start >= end)
		return;
	unsigned int l = start, r = end;
	quick_sort_partition(keys, values, start, end, l, r, sorter);
	quick_sort(keys, values, start, r, sorter);
	quick_sort(keys, values, l, end, sorter);
}

template<typename T>
inline void quick_sort(T* keys, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys, 0, length - 1);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort(T* keys, unsigned int length, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys, 0, length - 1, sorter);
}

template<typename K, typename V>
inline void quick_sort(K* keys, V* values, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys, values, 0, length - 1);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort(K* keys, V* values, unsigned int length, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys, values, 0, length - 1, sorter);
}

template<typename T>
inline void quick_sort(array<T>& keys) {
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys.data, 0, (unsigned int) keys.length - 1);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort(array<T>& keys, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys.data, 0, (unsigned int) keys.length - 1, sorter);
}

template<typename K, typename V>
inline void quick_sort(array<K>& keys, array<V>& values) {
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys.data, values.data, 0, (unsigned int) keys.length - 1);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void quick_sort(array<K>& keys, array<V>& values, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "quick_sort WARNING: Length is zero.\n");
		return;
	}
#endif
	quick_sort(keys.data, values.data, 0, (unsigned int) keys.length - 1, sorter);
}


/**
 * Hybrid quicksort-insertion sort.
 */

template<typename T>
void sort(T* array, unsigned int start, unsigned int end)
{
	if (start >= end)
		return;
	else if (start + 16 >= end) {
		insertion_sort(&array[start], end - start + 1);
		return;
	}
	unsigned int l = start, r = end;
	quick_sort_partition(array, start, end, l, r);
	sort(array, start, r);
	sort(array, l, end);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void sort(T* array, unsigned int start, unsigned int end, const Sorter& sorter)
{
	if (start >= end)
		return;
	else if (start + 16 >= end) {
		insertion_sort(&array[start], end - start + 1, sorter);
		return;
	}
	unsigned int l = start, r = end;
	quick_sort_partition(array, start, end, l, r, sorter);
	sort(array, start, r, sorter);
	sort(array, l, end, sorter);
}

template<typename K, typename V>
void sort(K* keys, V* values, unsigned int start, unsigned int end)
{
	if (start >= end)
		return;
	else if (start + 16 >= end) {
		insertion_sort(&keys[start], &values[start], end - start + 1);
		return;
	}
	unsigned int l = start, r = end;
	quick_sort_partition(keys, values, start, end, l, r);
	sort(keys, values, start, r);
	sort(keys, values, l, end);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
void sort(K* keys, V* values, unsigned int start, unsigned int end, const Sorter& sorter)
{
	if (start >= end)
		return;
	else if (start + 16 >= end) {
		insertion_sort(&keys[start], &values[start], end - start + 1, sorter);
		return;
	}
	unsigned int l = start, r = end;
	quick_sort_partition(keys, values, start, end, l, r, sorter);
	sort(keys, values, start, r, sorter);
	sort(keys, values, l, end, sorter);
}

template<typename T>
inline void sort(T* keys, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys, 0, length - 1);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void sort(T* keys, unsigned int length, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys, 0, length - 1, sorter);
}

template<typename K, typename V>
inline void sort(K* keys, V* values, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys, values, 0, length - 1);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void sort(K* keys, V* values, unsigned int length, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys, values, 0, length - 1, sorter);
}

template<typename T>
inline void sort(array<T>& keys) {
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, 0, keys.length - 1);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void sort(array<T>& keys, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, 0, keys.length - 1, sorter);
}

template<typename K, typename V>
inline void sort(array<K>& keys, array<V>& values) {
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, values.data, 0, keys.length - 1);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value, Sorter>::type* = nullptr>
inline void sort(array<K>& keys, array<V>& values, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, values.data, 0, keys.length - 1, sorter);
}

struct dummy_sorter { };

template<typename T>
inline bool less_than(const T& first, const T& second, const dummy_sorter& sorter) {
	return (first < second);
}

/**
 * Deletes consecutive duplicates in the given array and
 * returns the new length.
 */
template<typename T>
unsigned int unique(T* array, size_t length)
{
	unsigned int result = 0;
	for (unsigned int i = 1; i < length; i++) {
		if (array[result] != array[i])
			array[++result] = array[i];
	}
	return result + 1;
}

template<typename T>
inline void unique(array<T>& a) {
	a.length = unique(a.data, a.length);
}

template<typename T>
void shuffle(T* array, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "shuffle WARNING: Length is zero.\n");
		return;
	}
#endif
	for (unsigned int i = length - 1; i > 0; i--) {
		unsigned int next = rand() % (i + 1);
		if (next != i)
			core::swap(array[next], array[i]);
	}
}

template<typename K, typename V>
void shuffle(K* keys, V* values, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "shuffle WARNING: Length is zero.\n");
		return;
	}
#endif
	for (unsigned int i = length - 1; i > 0; i--) {
		unsigned int next = rand() % (i + 1);
		if (next != i) {
			core::swap(keys[next], keys[i]);
			core::swap(values[next], values[i]);
		}
	}
}

template<typename K, typename V>
struct pair {
	K key;
	V value;

	pair(const K& key, const V& value) : key(key), value(value) { }
};

template<typename K, typename V>
inline bool operator < (const pair<K, V>& first, const pair<K, V>& second) {
	return first.key < second.key;
}

template<typename K, typename V>
inline bool operator <= (const pair<K, V>& first, const pair<K, V>& second) {
	return first.key <= second.key;
}

template<typename K, typename V>
inline bool operator > (const pair<K, V>& first, const pair<K, V>& second) {
	return first.key > second.key;
}

template<typename K, typename V>
inline bool operator >= (const pair<K, V>& first, const pair<K, V>& second) {
	return first.key >= second.key;
}

template<typename K, typename V>
inline void swap(pair<K, V>& first, pair<K, V>& second) {
	swap(first.key, second.key);
	swap(first.value, second.value);
}

inline void array_test(void)
{
	array<char> buf(1);
	buf.append("0123456789 ", 11);

	buf[(unsigned int) buf.length] = '\0';
	if (buf.length != 11)
		fprintf(stderr, "array test ERROR: First array length test failed.\n");
	if (buf.capacity != 16)
		fprintf(stderr, "array test ERROR: First array capacity test failed.\n");
	if (strcmp(buf.data, "0123456789 ") != 0)
		fprintf(stderr, "array test ERROR: First string comparison test failed.\n");

	buf.append("abcdefghijklmnopqrstuvwxyz ", 27);

	buf[(unsigned int) buf.length] = '\0';
	if (buf.length != 11 + 27)
		fprintf(stderr, "array test ERROR: Second array length test failed.\n");
	if (buf.capacity != 64)
		fprintf(stderr, "array test ERROR: Second array capacity test failed.\n");
	if (strcmp(buf.data, "0123456789 abcdefghijklmnopqrstuvwxyz ") != 0)
		fprintf(stderr, "array test ERROR: Second string comparison test failed.\n");

	buf.append("9876543210 ", 11);

	buf[(unsigned int) buf.length] = '\0';
	if (buf.length != 11 + 27 + 11)
		fprintf(stderr, "array test ERROR: Third array length test failed.\n");
	if (buf.capacity != 64)
		fprintf(stderr, "array test ERROR: Third array capacity test failed.\n");
	if (strcmp(buf.data, "0123456789 abcdefghijklmnopqrstuvwxyz 9876543210 ") != 0)
		fprintf(stderr, "array test ERROR: Third string comparison test failed.\n");

	/* test some of the helper functions */
	array<int> numbers = array<int>(10);
	numbers.add(4);
	numbers.add(-6);
	numbers.add(4);
	numbers.add(2);
	numbers.add(0);
	numbers.add(-6);
	numbers.add(1);
	numbers.add(4);
	numbers.add(2);

	array<int> numbers_copy = array<int>(10);
	numbers_copy.append(numbers.data, numbers.length);
	insertion_sort(numbers);
	quick_sort(numbers_copy);

	int expected[] = {-6, -6, 0, 1, 2, 2, 4, 4, 4};
	for (unsigned int i = 0; i < 9; i++) {
		if (numbers[i] != expected[i]) {
			fprintf(stderr, "array test ERROR: insertion_sort failed.\n");
			return;
		}
		if (numbers_copy[i] != expected[i]) {
			fprintf(stderr, "array test ERROR: quick_sort failed.\n");
			return;
		}
	}

	int expected_unique[] = {-6, 0, 1, 2, 4};
	unique(numbers);
	if (numbers.length != 5) {
		fprintf(stderr, "array test ERROR: unique failed.\n");
		return;
	}
	for (unsigned int i = 0; i < 5; i++) {
		if (numbers[i] != expected_unique[i]) {
			fprintf(stderr, "array test ERROR: unique failed.\n");
			return;
		}
	}

	printf("array test completed.\n");
}

} /* namespace core */

#endif /* ARRAY_H_ */
