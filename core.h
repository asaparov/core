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
inline constexpr size_t array_length(T (&array)[N]) {
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

template<typename T>
inline void move(const T& src, T& dst) {
	T::move(src, dst);
}

template<typename T>
inline void move(T* a, T*& b) {
	b = a;
}

inline void move(const bool a, bool& b) {
	b = a;
}

inline void move(const int a, int& b) {
	b = a;
}

inline void move(const unsigned int a, unsigned int& b) {
	b = a;
}

inline void move(const long a, long& b) {
	b = a;
}

inline void move(const unsigned long a, unsigned long& b) {
	b = a;
}

inline void move(const unsigned long long a, unsigned long long& b) {
	b = a;
}

inline void move(const short a, short& b) {
	b = a;
}

inline void move(const unsigned short a, unsigned short& b) {
	b = a;
}

inline void move(const char a, char& b) {
	b = a;
}

inline void move(const unsigned char a, unsigned char& b) {
	b = a;
}

inline void move(const float a, float& b) {
	b = a;
}

inline void move(const double a, double& b) {
	b = a;
}

template<typename T>
inline void swap(T& a, T& b) {
	T::swap(a, b);
}

template<typename T>
inline void swap(T*& a, T*& b) {
	T* temp = a;
	a = b;
	b = temp;
}

inline void swap(bool& a, bool& b) {
	bool temp = a;
	a = b;
	b = temp;
}

inline void swap(int& a, int& b) {
	int temp = a;
	a = b;
	b = temp;
}

inline void swap(unsigned int& a, unsigned int& b) {
	unsigned int temp = a;
	a = b;
	b = temp;
}

inline void swap(long& a, long& b) {
	long temp = a;
	a = b;
	b = temp;
}

inline void swap(unsigned long& a, unsigned long& b) {
	unsigned long temp = a;
	a = b;
	b = temp;
}

inline void swap(unsigned long long& a, unsigned long long& b) {
	unsigned long long temp = a;
	a = b;
	b = temp;
}

inline void swap(short& a, short& b) {
	short temp = a;
	a = b;
	b = temp;
}

inline void swap(unsigned short& a, unsigned short& b) {
	unsigned short temp = a;
	a = b;
	b = temp;
}

inline void swap(char& a, char& b) {
	char temp = a;
	a = b;
	b = temp;
}

inline void swap(unsigned char& a, unsigned char& b) {
	unsigned char temp = a;
	a = b;
	b = temp;
}

inline void swap(float& a, float& b) {
	float temp = a;
	a = b;
	b = temp;
}

inline void swap(double& a, double& b) {
	double temp = a;
	a = b;
	b = temp;
}

inline constexpr long unsigned int size_of(bool a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(int a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(unsigned int a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(long a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(unsigned long a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(unsigned long long a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(short a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(unsigned short a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(char a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(unsigned char a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(float a) {
	return sizeof(a);
}

inline constexpr long unsigned int size_of(double a) {
	return sizeof(a);
}

template<typename E,
	typename std::enable_if<std::is_enum<E>::value>::type* = nullptr>
inline constexpr long unsigned int size_of(const E& a) {
	return sizeof(a);
}

struct dummy_metric { };

template<typename T,
	typename std::enable_if<!std::is_pointer<T>::value>::type* = nullptr,
	typename std::enable_if<!std::is_enum<T>::value>::type* = nullptr>
inline long unsigned int size_of(const T& a) {
	return T::size_of(a, dummy_metric());
}

template<typename T>
inline constexpr long unsigned int size_of(const T* const& a) {
	return sizeof(T*);
}

template<typename T, size_t N>
inline long unsigned int size_of(T (&array)[N]) {
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
		return (key == 0);
	}

	static inline void set_empty(K& key) {
		key = 0;
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
