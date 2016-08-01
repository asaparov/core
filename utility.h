/*
 * utility.h
 *
 *  Created on: Jan 8, 2014
 *      Author: asaparov
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <stdio.h>
#include "io.h"

#if defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#undef max
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#if defined(_WIN32)
static uint32_t __inline __builtin_clz(uint32_t x) {
	unsigned long r = 0;
	_BitScanReverse(&r, x);
	return (31 - r);
}
#endif

namespace core {

struct string {
	unsigned int length;
	char* data;

	string() { }

	string(const char* src) {
		if (!initialize(src, (unsigned int) strlen(src)))
			exit(EXIT_FAILURE);
	}

	string(const char* src, unsigned int length) {
		if (!initialize(src, length))
			exit(EXIT_FAILURE);
	}

	string(unsigned int length) {
		if (!initialize(length))
			exit(EXIT_FAILURE);
	}

	~string() {
		core::free(data);
	}

	inline char& operator [] (unsigned int index) {
		return data[index];
	}

	inline const char& operator [] (unsigned int index) const {
		return data[index];
	}

	inline void operator = (const string& s) {
		initialize(s.data, s.length);
	}

	inline void operator += (const char* src) {
		unsigned int src_length = (unsigned int) strlen(src);
		char* new_data = (char*) realloc(data, sizeof(char) * (length + src_length));
		if (new_data == NULL) {
			fprintf(stderr, "string.operator += ERROR: Unable to expand string.\n");
			exit(EXIT_FAILURE);
		}

		data = new_data;
		memcpy(data + length, src, sizeof(char) * src_length);
		length += src_length;
	}

	inline bool operator < (const string& other) const {
		for (unsigned int i = 0; ; i++) {
			if (i == length) {
				if (i == other.length) return false;
				else return true;
			} else if (i == other.length) {
				return false;
			}

			if (data[i] > other.data[i])
				return false;
			else if (data[i] < other.data[i])
				return true;
		}
	}

	inline unsigned int index_of(char c) const {
		for (unsigned int i = 0; i < length; i++) {
			if (data[i] == c)
				return i;
		}
		return length;
	}

	static inline bool is_empty(const string& key) {
		return key.data == NULL;
	}

	static inline void set_empty(string& key) {
		key.data = NULL;
	}

	static inline void set_empty(string* keys, unsigned int length) {
		memset(keys, 0, sizeof(string) * length);
	}

	static inline unsigned int hash(const string& key) {
		return default_hash(key.data, key.length);
	}

	static inline void move(const string& src, string& dst) {
		dst.length = src.length;
		dst.data = src.data;
	}

	static inline bool copy(const string& src, string& dst) {
		return dst.initialize(src.data, src.length);
	}

	static inline void swap(string& first, string& second) {
		core::swap(first.length, second.length);
		core::swap(first.data, second.data);
	}

	template<typename Metric>
	static inline long unsigned int size_of(const string& str, const Metric& metric) {
		return core::size_of(str.length) + sizeof(char) * str.length;
	}

	static inline void free(string& str) {
		core::free(str.data);
	}

private:
	bool initialize(unsigned int src_length) {
		length = src_length;
		data = (char*) malloc(sizeof(char) * (length == 0 ? 1 : length));
		if (data == NULL) {
			fprintf(stderr, "string.initialize ERROR: Unable to initialize string.\n");
			return false;
		}
		return true;
	}

	bool initialize(const char* src, unsigned int src_length) {
		if (!initialize(src_length))
			return false;
		memcpy(data, src, sizeof(char) * length);
		return true;
	}

	friend bool string_init(string&, const char*, unsigned int);
	friend bool string_init(string&, unsigned int);
};

inline bool string_init(string& dest, const char* src, unsigned int length) {
	return dest.initialize(src, length);
}

inline bool string_init(string& dest, const string& src) {
	return string_init(dest, src.data, src.length);
}

inline bool string_init(string& dest, unsigned int length) {
	return dest.initialize(length);
}

inline bool read(string& s, FILE* in) {
	if (!read(s.length, in)) return false;
	s.data = (char*) malloc(sizeof(char) * s.length);
	if (s.data == NULL)
		return false;
	return read(s.data, in, s.length);
}

inline bool write(const string& s, FILE* out) {
	if (!write(s.length, out)) return false;
	return write(s.data, out, s.length);
}

template<typename Stream>
inline bool print(const string& s, Stream& stream) {
	return fwrite(s.data, sizeof(char), s.length, stream) == s.length;
}

inline bool operator == (const string& first, const char* second) {
	for (unsigned int i = 0; i < first.length; i++) {
		if (first[i] != second[i])
			return false;
	}
	if (second[first.length] != '\0')
		return false;
	return true;
}

inline bool operator == (const char* first, const string& second) {
	return (second == first);
}

inline bool operator == (const string& first, const string& second) {
	if (first.length != second.length) return false;
	/* we are guaranteed that only the first may be uninitialized */
	if (first.data == NULL)
		return false;
	return memcmp(first.data, second.data, first.length * sizeof(char)) == 0;
}

inline bool operator != (const string& first, const char* second) {
	return !(first == second);
}

inline bool operator != (const char* first, const string& second) {
	return !(second == first);
}

inline bool operator != (const string& first, const string& second) {
	if (first.length != second.length) return true;
	if (first.data == NULL) {
		if (second.data == NULL) return true;
		else return false;
	} else if (second.data == NULL)
		return false;
	return memcmp(first.data, second.data, first.length * sizeof(char)) != 0;
}

/* NOTE: this function assumes the argument is non-zero */
inline unsigned int log2(unsigned int x) {
	return (unsigned int) sizeof(unsigned int) * 8 - __builtin_clz(x) - 1;
}

inline FILE* open_file(const char* filename, const char* mode) {
#if defined(_WIN32)
	FILE* file;
	if (fopen_s(&file, filename, mode) != 0)
		return NULL;
	return file;
#else
	return fopen(filename, mode);
#endif
}

inline char* read_file(const char* filename,
		size_t& bytes_read, bool append_null)
{
	FILE* fin = open_file(filename, "r");
	if (fseek(fin, 0, SEEK_END) != 0)
		return NULL;

	long int filesize = ftell(fin);
	if (filesize == -1L)
		return NULL;

	if (append_null)
		filesize++;
	char* data = (char*) malloc(sizeof(char) * filesize);
	if (data == NULL)
		return NULL;
	if (fseek(fin, 0, SEEK_SET) != 0)
		return NULL;
	bytes_read = fread(data, sizeof(char), filesize, fin);
	if (append_null)
		data[filesize - 1] = '\0';
	return data;
}

/* see: http://stackoverflow.com/questions/306533/how-do-i-get-a-list-of-files-in-a-directory-in-c */
inline bool get_files_in_directory(array<string>& out, const char* directory)
{
#if defined(_WIN32)
	HANDLE dir;
	WIN32_FIND_DATA file_data;

	size_t required;
	if (mbstowcs_s(&required, NULL, 0, directory, 0) != 0)
		return false;
	wchar_t* dir_prefix = (wchar_t*) malloc(sizeof(wchar_t) * (required + 4));
	if (dir_prefix == NULL) {
		fprintf(stderr, "get_files_in_directory ERROR: Out of memory.\n");
		return false;
	}
	if (mbstowcs_s(&required, dir_prefix, required + 4, directory, required) != 0) {
		free(dir_prefix);
		return false;
	}
	dir_prefix[required] = '/';
	dir_prefix[required + 1] = '*';
	dir_prefix[required + 2] = ' ';
	dir_prefix[required + 3] = '\0';
	if ((dir = FindFirstFile(dir_prefix, &file_data)) == INVALID_HANDLE_VALUE) {
		free(dir_prefix);
		return false;
	}
	free(dir_prefix);

	do {
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if (file_data.cFileName[0] == '.' || is_directory)
			continue;

		if (!out.ensure_capacity(out.length + 1))
			return false;

		if (wcstombs_s(&required, NULL, 0, file_data.cFileName, 0) != 0
		 || !string_init(out[(unsigned int) out.length], (unsigned int) required + 1))
			return false;
		out.length++;
		if (wcstombs_s(&required, out.last().data, required + 1, file_data.cFileName, wcslen(file_data.cFileName)) != 0)
			return false;
	} while (FindNextFile(dir, &file_data));

    FindClose(dir);
    return true;
#else
    DIR* dir;
    dirent* ent;
    struct stat st;

    dir = opendir(directory);
    if (dir == NULL)
    	return false;
    while ((ent = readdir(dir)) != NULL) {
        string full_file_name = string(directory);
        full_file_name += "/";
        full_file_name += ent->d_name;
        full_file_name += " ";
        full_file_name[full_file_name.length - 1] = '\0';
        if (ent->d_name[0] == '.' || stat(full_file_name.data, &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;
        if (is_directory)
            continue;

        if (!out.ensure_capacity(out.length + 1))
        	return false;
        string_init(out[out.length], ent->d_name);
        out.length++;
    }
    closedir(dir);
    return true;
#endif
}

} /* namespace core */

#endif /* UTILITY_H_ */
