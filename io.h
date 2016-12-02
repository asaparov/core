/**
 * io.h
 *
 *  Created on: Aug 29, 2014
 *      Author: asaparov
 */

#ifndef IO_H_
#define IO_H_

#include "array.h"
#include "map.h"

#include <stdarg.h>
#include <stdint.h>
#include <cwchar>
#include <errno.h>

using namespace core;

namespace detail {
	template<typename C> static auto test_readable(int) ->
			decltype(size_t(fread(std::declval<void*>(), 1, 1, std::declval<C>())), std::true_type{});
	template<typename C> static auto test_readable(long) -> std::false_type;

	template<typename C> static auto test_writeable(int) ->
			decltype(size_t(fwrite(std::declval<const void*>(), 1, 1, std::declval<C>())), std::true_type{});
	template<typename C> static auto test_writeable(long) -> std::false_type;

	template<typename C> static auto test_printable(int) ->
			decltype(int(fprintf(std::declval<C>(), " ")), std::true_type{});
	template<typename C> static auto test_printable(long) -> std::false_type;
}

template<typename T> struct is_readable : decltype(detail::test_readable<T>(0)){};
template<typename T> struct is_writeable : decltype(detail::test_writeable<T>(0)){};
template<typename T> struct is_printable : decltype(detail::test_printable<T>(0)){};

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool read(T& value, FILE* in) {
	return (fread(&value, sizeof(T), 1, in) == 1);
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool read(T* values, FILE* in, unsigned int length) {
	return (fread(values, sizeof(T), length, in) == length);
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool write(const T& value, FILE* out) {
	return (fwrite(&value, sizeof(T), 1, out) == 1);
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool write(const T* values, FILE* out, unsigned int length) {
	return (fwrite(values, sizeof(T), length, out) == length);
}

inline bool print(const char& value, FILE* out) {
	return (fputc(value, out) != EOF);
}

inline bool print(const unsigned int& value, FILE* out) {
	return (fprintf(out, "%u", value) > 0);
}

inline bool print(const unsigned long& value, FILE* out) {
	return (fprintf(out, "%lu", value) > 0);
}

inline bool print(const unsigned long long& value, FILE* out) {
	return (fprintf(out, "%llu", value) > 0);
}

inline bool print(const float& value, FILE* out) {
	return (fprintf(out, "%f", (double) value) > 0);
}

inline bool print(const double& value, FILE* out) {
	return (fprintf(out, "%lf", value) > 0);
}

inline bool print(const char* values, FILE* out) {
	return (fprintf(out, "%s", values) > 0);
}

struct memory_stream {
	unsigned int length;
	unsigned int position;
	char* buffer;

	std::mbstate_t shift; /* for wide character operations (such as reading UTF8 symbols using fgetwc) */

	memory_stream() { }

	memory_stream(unsigned int initial_capacity) : length(initial_capacity), position(0) {
		buffer = (char*) malloc(sizeof(char) * length);
		if (buffer == NULL) {
			fprintf(stderr, "memory_stream ERROR: Unable to initialize buffer.\n");
			exit(EXIT_FAILURE);
		}
	}

	memory_stream(const char* buf, unsigned int length) : length(length), position(0) {
		buffer = (char*) malloc(sizeof(char) * length);
		if (buffer == NULL) {
			fprintf(stderr, "memory_stream ERROR: Unable to initialize buffer.\n");
			exit(EXIT_FAILURE);
		}
		memcpy(buffer, buf, sizeof(char) * length);
	}

	~memory_stream() {
		free(buffer);
	}

	inline bool read(void* dst, unsigned int bytes) {
		if (position + bytes >= length)
			return false;
		memcpy(dst, buffer + position, bytes);
		position += bytes;
		return true;
	}

	inline bool ensure_capacity(unsigned int bytes) {
		if (position + bytes <= length)
			return true;

		unsigned int new_length = length;
		if (!core::expand(buffer, new_length, position + bytes))
			return false;
		length = new_length;
		return true;
	}

	inline bool write(const void* src, unsigned int bytes) {
		if (!ensure_capacity(bytes))
			return false;
		memcpy(buffer + position, src, bytes);
		position += bytes;
		return true;
	}
};

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool read(T& value, memory_stream& in) {
	return in.read(&value, sizeof(T));
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool read(T* values, memory_stream& in, unsigned int length) {
	return in.read(values, (unsigned int) sizeof(T) * length);
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool write(const T& value, memory_stream& out) {
	return out.write(&value, sizeof(T));
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
inline bool write(const T* values, memory_stream& out, unsigned int length) {
	return out.write(values, (unsigned int) sizeof(T) * length);
}

inline size_t fwrite(const void* src, size_t size, size_t n, memory_stream& out) {
	if (out.write(src, (unsigned int) (size * n)))
		return n;
	else return 0;
}

inline wint_t fgetwc(memory_stream& out) {
	wchar_t c;
	size_t length = mbrtowc(&c, out.buffer + out.position, out.length - out.position, &out.shift);
	if (length == 0) {
		return WEOF;
	} else if (length == static_cast<std::size_t>(-1) || length == static_cast<std::size_t>(-2)) {
		errno = EILSEQ;
		return WEOF;
	}

	out.position += (unsigned int) length;
	return c;
}

inline int fputc(int c, memory_stream& out) {
	char ch = (char) c;
	if (out.write(&ch, sizeof(char)))
		return c;
	else return EOF;
}

inline int fputs(const char* s, memory_stream& out) {
	if (out.write(s, (unsigned int) strlen(s)))
		return 1;
	else return EOF;
}

inline int fprintf(memory_stream& out, const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);

#if defined(_WIN32)
	int required = _vscprintf(format, argptr);
	if (!out.ensure_capacity(required + 1)) {
		fprintf(stderr, "fprintf ERROR: Unable to expand memory stream buffer.\n");
		va_end(argptr);
		return -1;
	}

	int written = vsnprintf_s(out.buffer + out.position, out.length - out.position, required, format, argptr);
#else
	int written = vsnprintf(out.buffer + out.position, out.length - out.position, format, argptr);
	if (written < 0) {
		va_end(argptr);
		return -1;
	} else if ((unsigned) written < out.length - out.position) {
		va_end(argptr);
		out.position += written;
		return written;
	}

	if (!out.ensure_capacity(written + 1)) {
		fprintf(stderr, "fprintf ERROR: Unable to expand memory stream buffer.\n");
		va_end(argptr);
		return -1;
	}
	written = vsnprintf(out.buffer + out.position, out.length - out.position, format, argptr);
#endif

	va_end(argptr);
	if (written < 0) return -1;
	out.position += written;
	return written;
}

template<typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const char* values, Stream& out) {
	return write(values, out, strlen(values));
}

struct dummy_scribe { };

template<typename T, typename Stream,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(T& value, Stream& in, dummy_scribe& scribe) {
	return read(value, in);
}

template<typename T, typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const T& value, Stream& out, dummy_scribe& scribe) {
	return write(value, out);
}

template<typename T, typename Stream,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
inline bool print(const T& value, Stream& out, dummy_scribe& scribe) {
	return print(value, out);
}

template<typename T, char LeftBracket = '[', char RightBracket = ']',
	typename SizeType, typename Stream, typename Printer,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
bool print(const T* values, SizeType length, Stream& out, Printer& printer) {
	if (!print(LeftBracket, out)) return false;
	if (length == 0)
		return print(RightBracket, out);
	if (!print(values[0], out, printer)) return false;
	for (unsigned int i = 1; i < length; i++) {
		if (!print(", ", out) || !print(values[i], out, printer))
			return false;
	}
	return print(RightBracket, out);
}

template<typename T, char LeftBracket = '[', char RightBracket = ']', typename Stream,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
inline bool print(const T* values, unsigned int length, Stream& out) {
	dummy_scribe printer;
	return print<T, LeftBracket, RightBracket>(values, length, out, printer);
}

template<typename T, size_t N, char LeftBracket = '[',
	char RightBracket = ']', typename Stream, typename Printer,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
bool print(const T (&values)[N], Stream& out, Printer& printer) {
	if (!print(LeftBracket, out)) return false;
	if (N == 0)
		return print(RightBracket, out);
	if (!print(values[0], out, printer)) return false;
	for (unsigned int i = 1; i < N; i++) {
		if (!print(", ", out) || !print(values[i], out, printer))
			return false;
	}
	return print(RightBracket, out);
}

template<typename T, size_t N, char LeftBracket = '[',
	char RightBracket = ']', typename Stream,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
inline bool print(const T (&values)[N], Stream& out) {
	dummy_scribe printer;
	return print<T, N, LeftBracket, RightBracket>(values, out, printer);
}

template<typename T, typename Stream, typename... Reader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(T* a, Stream& in, unsigned int length, Reader&&... reader) {
	for (unsigned int i = 0; i < length; i++)
		if (!read(a[i], in, std::forward<Reader>(reader)...)) return false;
	return true;
}

template<typename T, typename Stream, typename... Reader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
bool read(array<T>& a, Stream& in, Reader&&... reader) {
	size_t length;
	if (!read(length, in))
		return false;
	size_t capacity = (length == 0) ? 1 : length;
	a.data = (T*) malloc(sizeof(T) * capacity);
	if (a.data == NULL) return false;
	if (!read(a.data, in, (unsigned int) length, std::forward<Reader>(reader)...)) {
		free(a.data);
		return false;
	}
	a.length = length;
	a.capacity = capacity;
	return true;
}

template<typename T, typename Stream, typename... Writer,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const T* a, Stream& out, unsigned int length, Writer&&... writer) {
	for (unsigned int i = 0; i < length; i++)
		if (!write(a[i], out, std::forward<Writer>(writer)...)) return false;
	return true;
}

template<typename T, typename Stream, typename... Writer,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
bool write(const array<T>& a, Stream& out, Writer&&... writer) {
	return write(a.length, out)
		&& write(a.data, out, (unsigned int) a.length, std::forward<Writer>(writer)...);
}

template<typename T, typename Stream, typename... Printer,
	typename std::enable_if<is_printable<Stream>::value>::type* = nullptr>
inline bool print(const array<T>& a, Stream& out, Printer&&... printer) {
	return print(a.data, a.length, out, std::forward<Printer>(printer)...);
}

template<typename T, typename Stream,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
bool read(hash_set<T>& set, Stream& in) {
	unsigned int length;
	if (!read(length, in)) return false;

	set.size = 0;
	set.capacity = RESIZE_THRESHOLD_INVERSE * (length == 0 ? 1 : length);
	set.keys = (T*) malloc(sizeof(T) * set.capacity);
	if (set.keys == NULL) return false;

	for (unsigned int i = 0; i < length; i++) {
		T key;
		if (!read(key, in)) return false;
		set.add(key);
	}
	return true;
}

template<typename T, typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
bool write(const hash_set<T>& set, Stream& out) {
	if (!write(set.size, out)) return false;
	for (unsigned int i = 0; i < set.capacity; i++) {
		if (is_empty(set.keys[i])) continue;
		if (!write(set.keys[i], out)) return false;
	}
	return true;
}

template<typename K, typename V, typename Stream,
	typename KeyReader, typename ValueReader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
bool read(hash_map<K, V>& map,
	Stream& in, alloc_keys_func alloc_keys,
	KeyReader& key_reader, ValueReader& value_reader)
{
	unsigned int length;
	if (!read(length, in)) return false;

	map.table.size = 0;
	map.table.capacity = RESIZE_THRESHOLD_INVERSE * (length == 0 ? 1 : length);
	map.table.keys = (K*) alloc_keys(map.table.capacity, sizeof(K));
	if (map.table.keys == NULL) return false;
	map.values = (V*) malloc(sizeof(V) * map.table.capacity);
	if (map.values == NULL) {
		free(map.table.keys);
		return false;
	}

	for (unsigned int i = 0; i < length; i++) {
		K key;
		if (!read(key, in, key_reader)) return false;

		bool contains;
		unsigned int bucket;
		map.get(key, contains, bucket);
		if (!contains) {
			if (!read(map.values[bucket], in, value_reader))
				return false;
			map.table.keys[bucket] = key;
			map.table.size++;
		} else if (!read(map.values[bucket], in, value_reader)) {
			return false;
		}
	}
	return true;
}

template<typename K, typename V, typename Stream, typename KeyReader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(hash_map<K, V>& map, Stream& in,
		KeyReader& key_reader,
		alloc_keys_func alloc_keys = calloc)
{
	dummy_scribe scribe;
	return read(map, in, alloc_keys, key_reader, scribe);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(hash_map<K, V>& map, Stream& in,
		alloc_keys_func alloc_keys = calloc)
{
	dummy_scribe scribe;
	return read(map, in, alloc_keys, scribe, scribe);
}

template<typename K, typename V, typename Stream,
	typename KeyWriter, typename ValueWriter,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
bool write(const hash_map<K, V>& map, Stream& out,
		KeyWriter& key_writer, ValueWriter& value_writer)
{
	if (!write(map.table.size, out)) return false;
	for (unsigned int i = 0; i < map.table.capacity; i++) {
		if (is_empty(map.table.keys[i])) continue;
		if (!write(map.table.keys[i], out, key_writer)
		 || !write(map.values[i], out, value_writer))
			return false;
	}
	return true;
}

template<typename K, typename V, typename Stream, typename KeyWriter,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const hash_map<K, V>& map, Stream& out, KeyWriter& key_writer) {
	dummy_scribe scribe;
	return write(map, out, key_writer, scribe);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const hash_map<K, V>& map, Stream& out) {
	dummy_scribe scribe;
	return write(map, out, scribe, scribe);
}

template<typename K, typename V, typename Stream,
	typename KeyReader, typename ValueReader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
bool read(array_map<K, V>& map, Stream& in,
		KeyReader& key_reader, ValueReader& value_reader)
{
	size_t length;
	if (!read(length, in)) return false;

	map.size = 0;
	map.capacity = (length == 0 ? 1 : length);
	map.keys = (K*) malloc(sizeof(K) * map.capacity);
	if (map.keys == NULL) return false;
	map.values = (V*) malloc(sizeof(V) * map.capacity);
	if (map.values == NULL) {
		free(map.keys);
		return false;
	}

	for (unsigned int i = 0; i < length; i++) {
		if (!read(map.keys[i], in, key_reader)) return false;
		if (!read(map.values[i], in, value_reader)) return false;
		map.size++;
	}
	return true;
}

template<typename K, typename V, typename Stream, typename KeyReader,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(array_map<K, V>& map, Stream& in, KeyReader& key_reader) {
	dummy_scribe scribe;
	return read(map, in, key_reader, scribe);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(array_map<K, V>& map, Stream& in) {
	dummy_scribe scribe;
	return read(map, in, scribe, scribe);
}

template<typename K, typename V, typename Stream,
	typename KeyWriter, typename ValueWriter,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
bool write(const array_map<K, V>& map, Stream& out,
		KeyWriter& key_writer, ValueWriter& value_writer)
{
	if (!write(map.size, out)) return false;
	for (unsigned int i = 0; i < map.size; i++) {
		if (!write(map.keys[i], out, key_writer)) return false;
		if (!write(map.values[i], out, value_writer)) return false;
	}
	return true;
}

template<typename K, typename V, typename Stream, typename KeyWriter,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const array_map<K, V>& map, Stream& out, KeyWriter& key_writer) {
	dummy_scribe scribe;
	return write(map, out, key_writer, scribe);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const array_map<K, V>& map, Stream& out) {
	dummy_scribe scribe;
	return write(map, out, scribe, scribe);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_readable<Stream>::value>::type* = nullptr>
inline bool read(pair<K, V>& p, Stream& stream)
{
	return read(p.key, stream) && read(p.value, stream);
}

template<typename K, typename V, typename Stream,
	typename std::enable_if<is_writeable<Stream>::value>::type* = nullptr>
inline bool write(const pair<K, V>& p, Stream& stream)
{
	return write(p.key, stream) && write(p.value, stream);
}

#endif /* IO_H_ */
