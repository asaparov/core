/**
 * \file core.h
 *
 * This file contains functions widely used by the core library. It contains
 * functions for object movement, swapping, copying, and hashing, as well as
 * the `free` function, which deinitializes objects and releases its
 * memory resources.
 *
 * Memory management
 * -----------------
 *
 * This library does not manage memory, and leaves it to the user to ensure
 * memory is allocated and freed correctly. The core data structures, such as
 * core::array, core::hash_set, core::hash_map, core::array_map do not
 * automatically free their elements upon deallocation. The library uses the
 * core::free() function to free memory resources. When developing a new type,
 * the class/struct should implement the public static function `free`, as in
 * the example below.
 *
 * ```{.cpp}
 * #include <core/core.h>
 * #include <stdio.h>
 * #include <string.h>
 * using namespace core;
 *
 * struct custom_string {
 * 	char* buffer;
 *
 * 	custom_string() { }
 *
 * 	custom_string(const char* src) {
 * 		buffer = (char*) malloc(sizeof(char) * (strlen(src) + 1));
 * 		memcpy(buffer, src, sizeof(char) * (strlen(src) + 1));
 * 	}
 *
 * 	~custom_string() {
 * 		core::free(buffer);
 * 	}
 *
 * 	static void free(custom_string& s) {
 * 		core::free(s.buffer);
 * 	}
 * };
 *
 * bool init(custom_string& s, const char* src) {
 * 	s.buffer = (char*) malloc(sizeof(char) * (strlen(src) + 1));
 * 	if (s.buffer == NULL)
 * 		return false;
 * 	memcpy(s.buffer, src, sizeof(char) * (strlen(src) + 1));
 * 	return true;
 * }
 *
 * int main() {
 * 	custom_string first = custom_string("first");
 * 	custom_string& second = *((custom_string*) alloca(sizeof(custom_string)));
 * 	init(second, "second");
 * 
 * 	printf("%s ", first.buffer);
 * 	printf("%s", second.buffer);
 * 
 * 	core::free(second);
 * }
 * ```
 * In this example, core::free() is not called on `first` since it was
 * initialized on the stack, and so its destructor will automatically free
 * `buffer`. However, this was not the case for `second`, and so we must call
 * core::free() directly. The expected output of the program is `first second`.
 *
 * <!-- Created on: Aug 29, 2015
 *          Author: asaparov -->
 */

#ifndef CORE_H_
#define CORE_H_

#include <type_traits>
#include <cstdlib>
#include <cstdint>
#include <utility>


namespace core {


/**
 * This function returns the compile-time length of the static array `array`.
 */
template<typename T, std::size_t N>
inline constexpr std::size_t array_length(const T (&array)[N]) {
	return N;
}

/**
 * Returns `first` if `first < second`, and returns `second` otherwise.
 */
template<typename T>
inline T min(const T& first, const T& second) {
	return (first < second) ? first : second;
}

/**
 * Returns `second` if `first < second`, and returns `first` otherwise.
 */
template<typename T>
inline T max(const T& first, const T& second) {
	return (first < second) ? second : first;
}

/**
 * If `T` satisfies [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * `dst` is assigned to `src`. Otherwise, this function calls
 * `T::move(src, dst)`. This function is intended to be used to effectively
 * move the object stored in `src` into the location specified by `dst`.
 */
template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type* = nullptr>
inline void move(const T& src, T& dst) {
	T::move(src, dst);
}

template<typename T>
inline void move(T* a, T*& b) {
	b = a;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline void move(const T& a, T& b) {
	b = a;
}

/**
 * If `T` satisfies [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * `dst` is assigned to `src`. Otherwise, this function calls and returns the
 * result of `T::copy(src, dst)`. This function is intended to be used to copy
 * the object stored in `src` into the location specified by `dst`.
 */
template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type* = nullptr>
inline bool copy(const T& src, T& dst) {
	return T::copy(src, dst);
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline bool copy(const T& a, T& b) {
	b = a;
	return true;
}

/**
 * If `T` satisfies [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * the values of `a` and `b` are swapped. Otherwise, this function calls
 * `T::swap(a, b)`. This function is intended to be used to swap the object
 * stored in `a` with the object stored in `b`.
 */
template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type* = nullptr>
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
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline void swap(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline constexpr T size_of(const T& a) {
	return sizeof(T);
}

template<typename E,
	typename std::enable_if<std::is_enum<E>::value>::type* = nullptr>
inline constexpr long unsigned int size_of(const E& a) {
	return sizeof(a);
}

struct default_metric { };

template<typename T,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_pointer<T>::value && !std::is_enum<T>::value>::type* = nullptr>
inline long unsigned int size_of(const T& a) {
	return T::size_of(a, default_metric());
}

template<typename T>
inline constexpr long unsigned int size_of(const T* const& a) {
	return sizeof(T*);
}

template<typename T, std::size_t N>
inline long unsigned int size_of(const T (&array)[N]) {
	long unsigned int size = 0;
	for (unsigned int i = 0; i < N; i++)
		size += size_of(array[i]);
	return size;
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline long unsigned int size_of(const T& a, const default_metric& metric) {
	return size_of(a);
}

template<typename T, typename Metric>
inline long unsigned int size_of(const T& a, const Metric& metric) {
	return T::size_of(a, metric);
}

template<typename T,
	typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type* = nullptr>
inline void free(T& a) { }

/**
 * If `T` satisfies [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * this function does nothing. Otherwise, this function calls
 * `T::free(a, std::forward<Args>(args)...)`. This function is intended to be
 * used to free the object stored in `a`, passing along any additional
 * arguments. Note that since `args` is variadic, it may also be empty.
 */
template<typename T, typename... Args,
	typename std::enable_if<!std::is_fundamental<T>::value && !std::is_enum<T>::value>::type* = nullptr>
inline void free(T& a, Args&&... args) {
	T::free(a, std::forward<Args>(args)...);
}

/**
 * This function calls [std::free](http://en.cppreference.com/w/c/memory/free)
 * on `a`.
 */
template<typename T>
inline void free(T* a) {
	::free(a);
}


/**
 * <!-- Some type traits. -->
 */

namespace detail {
	template<typename C> static auto test_swappable(int) ->
			decltype(void(core::swap(std::declval<C&>(), std::declval<C&>())), std::true_type{});
	template<typename C> static auto test_swappable(long) -> std::false_type;

	template<typename C> static auto test_moveable(int) ->
			decltype(void(core::move(std::declval<C&>(), std::declval<C&>())), std::true_type{});
	template<typename C> static auto test_moveable(long) -> std::false_type;

	template<typename C> static auto test_copyable(int) ->
			decltype(void(core::copy(std::declval<C&>(), std::declval<C&>())), std::true_type{});
	template<typename C> static auto test_copyable(long) -> std::false_type;
}

/**
 * This type trait is [true_type](http://en.cppreference.com/w/cpp/types/integral_constant)
 * if and only if `T` satisfies any of the following:
 * 	1. [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental),
 * 	2. [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * 	3. or implements the public static method `void T::move(const T&, T&)`.
 */
template<typename T> struct is_moveable : decltype(core::detail::test_moveable<T>(0)){};

/**
 * This type trait is [true_type](http://en.cppreference.com/w/cpp/types/integral_constant)
 * if and only if `T` satisfies any of the following:
 * 	1. [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental),
 * 	2. [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * 	3. or implements the public static method `void T::swap(T&, T&)`.
 */
template<typename T> struct is_swappable : decltype(core::detail::test_swappable<T>(0)){};

/**
 * This type trait is [true_type](http://en.cppreference.com/w/cpp/types/integral_constant)
 * if and only if `T` satisfies any of the following:
 * 	1. [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental),
 * 	2. [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * 	3. or implements the public static method `bool T::copy(const T&, T&)`.
 */
template<typename T> struct is_copyable : decltype(core::detail::test_copyable<T>(0)){};


/**
 * <!-- Common functionality for hashing. -->
 */

/* these are defined in map.h */
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)

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
struct hasher<K, typename std::enable_if<std::is_fundamental<K>::value || std::is_enum<K>::value>::type> {
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

/**
 * Hashtables in this library require the type `K` to define a special
 * "empty value" to indicate that a bucket is empty. For `K` that satisfies
 * [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * this function returns whether `key == static_cast<K>(0)`. Otherwise, this
 * function returns `K::is_empty(key)`. Thus, to enable the use of a custom
 * struct/class as a hashtable key, it must implement the public static
 * function `is_empty`.
 */
template<typename K>
inline bool is_empty(const K& key) {
	return hasher<K>::is_empty(key);
}

/**
 * Hashtables in this library require the type `K` to define a special
 * "empty value" to indicate that a bucket is empty. For `K` that satisfies
 * [is_fundamental](http://en.cppreference.com/w/cpp/types/is_fundamental)
 * or [is_enum](http://en.cppreference.com/w/cpp/types/is_enum),
 * this function sets `key` to `static_cast<K>(0)`. Otherwise, this function
 * calls `K::set_empty(key)`. Some hashtable operations use this operation, and
 * therefore, if a custom struct/class is used as a hashtable key, it must
 * implement the public static function `set_empty` in order to use the
 * aforementioned hashtable operations.
 */
template<typename K>
inline void set_empty(K& key) {
	hasher<K>::set_empty(key);
}


} /* namespace core */

#endif /* CORE_H_ */
