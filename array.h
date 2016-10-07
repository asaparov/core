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
#include <algorithm>

#include "core.h"

#define RESIZE_FACTOR 2

namespace core {

template<typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
inline bool resize(T*& data, const SizeType& new_capacity) {
	T* new_data = (T*) realloc(data, new_capacity * sizeof(T));
	if (new_data == NULL) {
		fprintf(stderr, "array.resize ERROR: Out of memory.\n");
		return false;
	}
	data = new_data;
	return true;
}

template<typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
inline bool expand(T*& data, SizeType& capacity, size_t new_length) {
	do {
		/* increase the size of the underlying array */
		capacity *= RESIZE_FACTOR;
	} while (new_length > capacity);
	return resize(data, capacity);
}

template<typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
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

template<typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
inline unsigned int index_of(const T& element, const T* data,
		const SizeType& length, const SizeType& start = 0)
{
	for (unsigned int i = start; i < length; i++)
		if (element == data[i])
			return i;
	return length;
}

template<typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
inline unsigned int last_index_of(const T& element, const T* data, const SizeType& length)
{
	unsigned int i = length;
	while (i != 0) {
		i--;
		if (element == data[i])
			return i;
	}
	return static_cast<unsigned int>(-1);
}

template<typename T>
struct array {
	T* data;
	size_t length;
	size_t capacity;

	array(size_t initial_capacity)
	{
		if (!initialize(initial_capacity))
			exit(EXIT_FAILURE);
	}

	~array() { free(); }

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
		core::move(data[length - 1], data[index]);
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
		return core::index_of(element, data, (unsigned int) length);
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

	static inline void free(array<T>& a) { a.free(); }

private:
	inline bool initialize(size_t initial_capacity)
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

	inline void free() {
		if (data != NULL) {
			core::free(data);
			data = NULL;
		}
	}

	template<typename K>
	friend bool array_init(array<K>& m, unsigned int initial_capacity);
};

template<typename T>
bool array_init(array<T>& m, unsigned int initial_capacity) {
	return m.initialize(initial_capacity);
}

template<typename T>
inline size_t size(const array<T>& m) {
	return m.length;
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

		while (hole > 0 && item < keys[hole - 1]) {
			keys[hole] = keys[hole - 1];
			hole--;
		}

		keys[hole] = item;
	}
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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

		while (hole > 0 && item < keys[hole - 1]) {
			keys[hole] = keys[hole - 1];
			values[hole] = values[hole - 1];
			hole--;
		}

		keys[hole] = item;
		values[hole] = value;
	}
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
inline void insertion_sort(array<T>& keys, const Sorter& sorter)
{
	insertion_sort(keys.data, (unsigned int) keys.length, sorter);
}

template<typename K, typename V>
inline void insertion_sort(array<K>& keys, array<V>& values) {
	insertion_sort(keys.data, values.data, (unsigned int) keys.length);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
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
	sort(keys.data, 0, (unsigned int) keys.length - 1);
}

template<typename T, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
inline void sort(array<T>& keys, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, 0, (unsigned int) keys.length - 1, sorter);
}

template<typename K, typename V>
inline void sort(array<K>& keys, array<V>& values) {
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, values.data, 0, (unsigned int) keys.length - 1);
}

template<typename K, typename V, typename Sorter,
	typename std::enable_if<!std::is_integral<Sorter>::value>::type* = nullptr>
inline void sort(array<K>& keys, array<V>& values, const Sorter& sorter)
{
#if !defined(NDEBUG)
	if (keys.length == 0) {
		fprintf(stderr, "sort WARNING: Length is zero.\n");
		return;
	}
#endif
	sort(keys.data, values.data, 0, (unsigned int) keys.length - 1, sorter);
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

/* forward declarations */

unsigned int sample_uniform(unsigned int);

template<typename T>
void shuffle(T* array, unsigned int length) {
#if !defined(NDEBUG)
	if (length == 0) {
		fprintf(stderr, "shuffle WARNING: Length is zero.\n");
		return;
	}
#endif
	for (unsigned int i = length - 1; i > 0; i--) {
		unsigned int next = sample_uniform(i + 1);
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
		unsigned int next = sample_uniform(i + 1);
		if (next != i) {
			core::swap(keys[next], keys[i]);
			core::swap(values[next], values[i]);
		}
	}
}

/**
 * Given sorted array a, this function finds the smallest
 * index i such that a[i] >= b and i >= start and i < end.
 */
template<typename T>
unsigned int linear_search(
	const T* a, const T& b,
	unsigned int start,
	unsigned int end)
{
	for (unsigned int i = start; i < end; i++)
		if (a[i] >= b) return i;
	return end;
}

/**
 * Given sorted array a, this function finds the smallest
 * index i such that a[i] > b and i >= start and i < end.
 */
template<typename T>
unsigned int strict_linear_search(
	const T* a, const T& b,
	unsigned int start,
	unsigned int end)
{
	for (unsigned int i = start; i < end; i++)
		if (a[i] > b) return i;
	return end;
}

/**
 * Given sorted array a, this function finds the smallest
 * index i such that a[i] > b and i >= start and i < end.
 */
template<typename T>
unsigned int reverse_strict_linear_search(
	const T* a, const T& b,
	unsigned int start,
	unsigned int end)
{
	for (unsigned int i = end; i > start; i--)
		if (a[i - 1] <= b) return i;
	return start;
}

/**
 * Given sorted array a, this function finds the smallest
 * index i such that a[i] >= b and i >= min and i <= max.
 */
/* TODO: implement a strict variant */
template<typename T>
unsigned int binary_search(
	const T* a, const T& b,
	unsigned int min,
	unsigned int max)
{
	if (a[max] < b)
		return max + 1;

	while (min < max) {
		unsigned int mid = (max + min) / 2;
		if (a[mid] < b)
			min = mid + 1;
		else max = mid;
	}

	return min;
}


/**
 * A simple pair data structure.
 */

template<typename K, typename V>
struct pair {
	K key;
	V value;

	pair(const K& key, const V& value) : key(key), value(value) { }

	inline bool operator == (const pair<K, V>& other) {
		return key == other.key && value == other.value;
	}

	static inline void move(const pair<K, V>& src, pair<K, V>& dst) {
		core::move(src.key, dst.key);
		core::move(src.value, dst.value);
	}

	static inline unsigned int hash(const pair<K, V>& pair) {
		return hasher<K>::hash(pair.key) + hasher<V>::hash(pair.value);
	}

	static inline bool is_empty(const pair<K, V>& pair) {
		return hasher<K>::is_empty(pair.key);
	}
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
constexpr pair<K, V> make_pair(const K& key, const V& value) {
	return pair<K, V>(key, value);
}

template<typename K, typename V>
inline void swap(pair<K, V>& first, pair<K, V>& second) {
	swap(first.key, second.key);
	swap(first.value, second.value);
}


/**
 * Functions for performing set operations with sorted arrays.
 * These functions assume the input arrays are sorted and
 * their elements are *distinct*.
 *
 * TODO: Extend these functions to non copy-assignable types.
 */

template<typename T>
void shift_right(T* list, unsigned int length, unsigned int index)
{
	for (unsigned int i = length; i > index; i--)
		move(list[i - 1], list[i]);
}

template<bool RemoveDuplicates, typename T, typename UnionFunc>
inline void set_union_helper(UnionFunc do_union, const T& item,
		unsigned int i, unsigned int j, const T*& prev)
{
	if (RemoveDuplicates) {
		if (prev == NULL || *prev != item) {
			do_union(item, i, j);
			prev = &item;
		}
	} else {
		do_union(item, i, j);
	}
}

template<typename T, typename UnionBoth,
	typename UnionFirst, typename UnionSecond, bool RemoveDuplicates = true>
void set_union(UnionBoth union_both,
	UnionFirst union_first, UnionSecond union_second,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	const T* prev = NULL;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			set_union_helper<RemoveDuplicates>(union_both, first[i], i, j, prev);
			i++; j++;
		} else if (first[i] < second[j]) {
			set_union_helper<RemoveDuplicates>(union_first, first[i], i, j, prev);
			i++;
		} else {
			set_union_helper<RemoveDuplicates>(union_second, second[j], i, j, prev);
			j++;
		}
	}

	while (i < first_length) {
		set_union_helper<RemoveDuplicates>(union_first, first[i], i, j, prev);
		i++;
	} while (j < second_length) {
		set_union_helper<RemoveDuplicates>(union_second, second[j], i, j, prev);
		j++;
	}
}

template<bool RemoveDuplicates, typename T, typename SizeType,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
inline void set_union_helper(T* dst, SizeType& dst_length, const T& item) {
	if (!RemoveDuplicates || dst_length == 0 || dst[dst_length - 1] != item) {
		dst[dst_length] = item;
		dst_length++;
	}
}

template<typename T, typename SizeType, bool RemoveDuplicates = true,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
void set_union(T* dst, SizeType& dst_length,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			set_union_helper<RemoveDuplicates>(dst, dst_length, first[i]);
			i++; j++;
		} else if (first[i] < second[j]) {
			set_union_helper<RemoveDuplicates>(dst, dst_length, first[i]);
			i++;
		} else {
			set_union_helper<RemoveDuplicates>(dst, dst_length, second[j]);
			j++;
		}
	}

	while (i < first_length) {
		set_union_helper<RemoveDuplicates>(dst, dst_length, first[i]);
		i++;
	} while (j < second_length) {
		set_union_helper<RemoveDuplicates>(dst, dst_length, second[j]);
		j++;
	}
}

template<typename T, bool RemoveDuplicates = true>
inline bool set_union(array<T>& dst,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	if (!dst.ensure_capacity(dst.length + first_length + second_length))
		return false;
	set_union<T, size_t, RemoveDuplicates>(dst.data, dst.length, first, first_length, second, second_length);
	return true;
}

template<typename T, bool RemoveDuplicates = true>
inline bool set_union(array<T>& dst, const array<T>& first, const array<T>& second) {
	return set_union<T, RemoveDuplicates>(dst,
		first.data, (unsigned int) first.length,
		second.data, (unsigned int) second.length);
}

template<typename T>
struct array_position {
	unsigned int array_id;
	unsigned int position;
	T* element;
};

/* NOTE: this function assumes the given arrays are all non-empty */
template<typename T, typename ArraySetCollection>
bool set_union(array<T>& dst, const ArraySetCollection& arrays, unsigned int array_count)
{
	/* first ensure the destination array has enough space */
	unsigned int total_size = dst.length;
	for (unsigned int i = 0; i < array_count; i++) {
#if !defined(NDEBUG)
		if (size(arrays[i]) == 0)
			fprintf(stderr, "set_union WARNING: Input array %u is empty.\n", i);
#endif
		total_size += size(arrays[i]);
	}
	if (!dst.ensure_capacity(total_size))
		return false;

	/* TODO: we can probably use a faster heap structure */
	array_position<T>* heap = (array_position<T>*) malloc(array_count * sizeof(array_position<T>));
	if (heap == NULL) {
		fprintf(stderr, "set_union ERROR: Out of memory.\n");
		return false;
	}
	for (unsigned int i = 0; i < array_count; i++)
		heap[i] = { i, 0, &arrays[i][0] };
	std::make_heap(heap, heap + array_count);

	/* add the first item to the destination set */
	unsigned int heap_size = array_count;
	std::pop_heap(heap, heap + heap_size);
	const array_position<T>& first = heap[heap_size];
	dst.data[dst.length] = first.key;
	dst.length++;
	if (size(arrays[first.value]) > 1) {
		heap[heap_size] = { first.array_id, 1, &arrays[first.array_id][1] };
		std::push_heap(heap, heap + heap_size);
	} else { heap_size--; }

	while (heap_size > 0)
	{
		std::pop_heap(heap, heap + heap_size);
		const array_position<T>& next = heap[heap_size];
		if (next.key != dst.last()) {
			dst.data[dst.length] = next.key;
			dst.length++;
		}
		if (next.value + 1 < size(arrays[next.value])) {
			heap[heap_size] = { next.array_id, next.position + 1, arrays[next.array_id][next.position + 1] };
			std::push_heap(heap, heap + heap_size);
		} else { heap_size--; }
	}
	free(heap);
	return true;
}

template<typename T, typename SizeType, bool BinarySearch = false,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
bool set_intersect(
	T* intersection, SizeType& intersection_length,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			intersection[intersection_length] = first[i];
			intersection_length++;
			i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				i = binary_search(first, second[j], i, first_length - 1);
			} else {
				i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}
	return true;
}

template<typename T, bool BinarySearch = false>
inline bool set_intersect(
	array<T>& intersection,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	if (!intersection.ensure_capacity(intersection.length + max(first_length, second_length)))
		return false;

	return set_intersect<T, decltype(intersection.length), BinarySearch>(
		intersection.data, intersection.length,
		first, first_length, second, second_length);
}

template<typename T, bool BinarySearch = false>
inline bool set_intersect(
	array<T>& intersection,
	const array<T>& first,
	const array<T>& second)
{
	return set_intersect<T, BinarySearch>(intersection,
		first.data, (unsigned int) first.length,
		second.data, (unsigned int) second.length);
}

/* in-place variant of set_intersect */
template<typename T, typename SizeType, bool BinarySearch = false,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
void set_intersect(
	T* first, SizeType& first_length,
	const T* second, unsigned int second_length)
{
	unsigned int index = 0;
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			first[index] = first[i];
			index++; i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				i = binary_search(first, second[j], i, first_length - 1);
			} else {
				i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}
	first_length = index;
}

template<typename T, bool BinarySearch = false>
inline void set_intersect(array<T>& first,
	const T* second, unsigned int second_length)
{
	return set_intersect<T, decltype(first.length), BinarySearch>(
			first.data, first.length, second, second_length);
}

template<typename T, bool BinarySearch = false>
inline void set_intersect(array<T>& first, const array<T>& second) {
	return set_intersect<T, BinarySearch>(first, second.data, second.length);
}

template<typename T, bool BinarySearch = false>
bool is_intersection_empty(
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			return false;
			i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				i = binary_search(first, second[j], i, first_length - 1);
			} else {
				i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}
	return true;
}

template<typename T, bool BinarySearch = false>
inline bool is_intersection_empty(const array<T>& first, const array<T>& second) {
	return is_intersection_empty<T, BinarySearch>(
		first.data, (unsigned int) first.length,
		second.data, (unsigned int) second.length);
}

template<typename T, bool BinarySearch = false>
bool is_subset(
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			i++; j++;
		} else if (first[i] < second[j]) {
			return false;
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}
	return (i == first_length);
}

template<typename T, typename EmitFunction, bool BinarySearch = false>
void set_subtract(EmitFunction emit,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				unsigned int next_i = binary_search(first, second[j], i, first_length - 1);
				for (; i < next_i; i++)
					emit(i);
				i = next_i;
			} else {
				emit(i);
				i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}

	while (i < first_length) {
		emit(i);
		i++;
	}
}

template<typename T, typename SizeType, bool BinarySearch = false,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
void set_subtract(T* dst, SizeType& dst_length,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				unsigned int next_i = binary_search(first, second[j], i, first_length - 1);
				for (; i < next_i; i++) {
					dst[dst_length] = first[i];
					dst_length++;
				}
				i = next_i;
			} else {
				dst[dst_length] = first[i];
				dst_length++;
				i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}

	memcpy(dst + dst_length, first + i, (first_length - i) * sizeof(T));
	dst_length += first_length - i;
}

template<typename T, bool BinarySearch = false>
bool set_subtract(array<T>& dst,
	const T* first, unsigned int first_length,
	const T* second, unsigned int second_length)
{
	if (!dst.ensure_capacity(dst.length + max(first_length, second_length)))
		return false;
	set_subtract(dst.data, dst.length, first, first_length, second, second_length);
	return true;
}

template<typename T, bool BinarySearch = false>
inline bool set_subtract(array<T>& dst,
	const array<T>& first,
	const array<T>& second)
{
	return set_subtract<T, BinarySearch>(dst,
		first.data, (unsigned int) first.length,
		second.data, (unsigned int) second.length);
}

/* in-place variant of set_subtract */
template<typename T, typename SizeType, bool BinarySearch = false,
	typename std::enable_if<std::is_integral<SizeType>::value>::type* = nullptr>
void set_subtract(
	T* first, SizeType& first_length,
	const T* second, unsigned int second_length)
{
	unsigned int index = 0;
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j]) {
			i++; j++;
		} else if (first[i] < second[j]) {
			if (BinarySearch) {
				/* use binary search to find the value of i
				   such that first.data[i] >= second.data[j] */
				unsigned int next_i = binary_search(first, second[j], i, first_length - 1);
				for (; i < next_i; i++) {
					first[index] = first[i];
					index++;
				}
				i = next_i;
			} else {
				first[index] = first[i];
				index++; i++;
			}
		} else {
			if (BinarySearch) {
				/* use binary search to find the value of j
				   such that second.data[j] >= first.data[i] */
				j = binary_search(second, first[i], j, second_length - 1);
			} else {
				j++;
			}
		}
	}

	while (i < first_length) {
		first[index] = first[i];
		index++; i++;
	}
	first_length = index;
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
