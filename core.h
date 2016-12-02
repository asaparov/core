/**
 * core.h - Common routines such as movement and swapping of objects.
 *
 *  Created on: Aug 29, 2015
 *      Author: asaparov
 */

#ifndef CORE_H_
#define CORE_H_

#include <type_traits>

namespace core {


template<typename T, size_t N>
inline constexpr size_t array_length(const T (&array)[N]) {
	return N;
}

template<typename T>
inline T min(const T& first, const T& second) {
	return (first < second) ? first : second;
}

template<typename T>
inline T max(const T& first, const T& second) {
	return (first < second) ? second : first;
}

template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value>::type* = nullptr>
inline void move(const T& src, T& dst) {
	T::move(src, dst);
}

template<typename T>
inline void move(T* a, T*& b) {
	b = a;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline void move(const T& a, T& b) {
	b = a;
}

template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value>::type* = nullptr>
inline bool copy(const T& src, T& dst) {
	return T::copy(src, dst);
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool copy(const T& a, T& b) {
	b = a;
	return true;
}

template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value>::type* = nullptr>
inline void swap(T& a, T& b) {
	T::swap(a, b);
}

template<typename T>
inline void swap(T*& a, T*& b) {
	T* temp = a;
	a = b;
	b = temp;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline void swap(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline constexpr T size_of(const T& a) {
	return sizeof(T);
}

template<typename E,
	typename std::enable_if<std::is_enum<E>::value>::type* = nullptr>
inline constexpr long unsigned int size_of(const E& a) {
	return sizeof(a);
}

struct dummy_metric { };

template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_pointer<T>::value && !std::is_enum<T>::value>::type* = nullptr>
inline long unsigned int size_of(const T& a) {
	return T::size_of(a, dummy_metric());
}

template<typename T>
inline constexpr long unsigned int size_of(const T* const& a) {
	return sizeof(T*);
}

template<typename T, size_t N>
inline long unsigned int size_of(const T (&array)[N]) {
	long unsigned int size = 0;
	for (unsigned int i = 0; i < N; i++)
		size += size_of(array[i]);
	return size;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline long unsigned int size_of(const T& a, const dummy_metric& metric) {
	return size_of(a);
}

template<typename T, typename Metric>
inline long unsigned int size_of(const T& a, const Metric& metric) {
	return T::size_of(a, metric);
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline void free(T& a) { }

template<typename T, typename... Args,
	typename std::enable_if<!std::is_fundamental<T>::value>::type* = nullptr>
inline void free(T& a, Args&&... args) {
	T::free(a, std::forward<Args>(args)...);
}

template<typename T>
inline void free(T* a) {
	::free(a);
}


/**
 * Common functionality for hashing.
 */

/* these are defined in map.h */
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)

#include <stdint.h>

template<typename K>
inline uint_fast32_t default_hash(const K& key);

template<typename K>
inline uint_fast32_t default_hash(const K* keys, unsigned int length);

#else
template<typename K>
inline unsigned int default_hash(const K& key);

template<typename K>
inline unsigned int default_hash(const K* keys, unsigned int length);
#endif

template<typename K>
inline void set_all_empty(K* keys, unsigned int length) {
	memset(keys, 0, sizeof(K) * length);
}

template<typename K, class Enable = void>
struct hasher {
	static inline bool is_empty(const K& key) {
		return K::is_empty(key);
	}

	static inline void set_empty(K& key) {
		K::set_empty(key);
	}

	static inline void set_empty(K* keys, unsigned int length) {
		K::set_empty(keys, length);
	}

	static inline unsigned int hash(const K& key) {
		return K::hash(key);
	}
};

template<typename K>
struct hasher<K*> {
	static inline bool is_empty(const K* const& key) {
		return (key == NULL);
	}

	static inline void set_empty(K* const& key) {
		key = NULL;
	}

	static inline void set_empty(K* const* keys, unsigned int length) {
		set_all_empty(keys, length);
	}

	static inline unsigned int hash(const K* const& key) {
		return default_hash(key);
	}
};

template<typename K>
struct hasher<K, typename std::enable_if<std::is_fundamental<K>::value>::type> {
	static inline bool is_empty(const K& key) {
		return (key == static_cast<K>(0));
	}

	static inline void set_empty(K& key) {
		key = static_cast<K>(0);
	}

	static inline void set_empty(K* keys, unsigned int length) {
		set_all_empty(keys, length);
	}

	static inline unsigned int hash(const K& key) {
		return default_hash(key);
	}
};

template<typename K>
inline bool is_empty(const K& key) {
	return hasher<K>::is_empty(key);
}

template<typename K>
inline void set_empty(K& key) {
	hasher<K>::set_empty(key);
}


} /* namespace core */

#endif /* CORE_H_ */
