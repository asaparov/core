/**
 * random.h
 *
 *  Created on: Aug 15, 2016
 *      Author: asaparov
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include <random>
#include <sstream>

#include "timer.h"
#include "array.h"
#include "io.h"

namespace core {


#if defined(NDEBUG)
static unsigned int seed = 225383240; //(unsigned int) milliseconds(); // 209912433
#else
static unsigned int seed = 0;
#endif

static std::minstd_rand engine = std::minstd_rand(seed);

inline unsigned int get_seed() {
	return seed;
}

inline void set_seed(unsigned int new_seed) {
	engine.seed(new_seed);
	seed = new_seed;
}

template<typename Stream>
inline bool read_random_state(Stream& in)
{
	size_t length;
	if (!read(length, in)) return false;
	char* state = (char*) alloca(sizeof(char) * length);
	if (state == NULL || !read(state, in, (unsigned int) length))
		return false;

	std::stringstream buffer(std::string(state, length));
	buffer >> engine;
	return true;
}

template<typename Stream>
inline bool write_random_state(Stream& out) {
	std::stringstream buffer;
	buffer << engine;
	std::string data = buffer.str();
	return write(data.length(), out) && write(data.c_str(), out, (unsigned int) data.length());
}

template<typename V,
	typename std::enable_if<std::is_floating_point<V>::value>::type* = nullptr>
unsigned int sample_categorical(V* probability, unsigned int length)
{
#if !defined(NDEBUG)
	if (length == 0)
		fprintf(stderr, "sample_categorical WARNING: Specified length is zero.\n");
#endif

	for (unsigned int j = 1; j < length; j++)
		probability[j] += probability[j - 1];

	/* select the new table assignment */
	V random = probability[length - 1] * ((V) engine() / engine.max());
	unsigned int selected_table = length - 1;
	for (unsigned int j = 0; j < length; j++) {
		if (random < probability[j]) {
			selected_table = j;
			break;
		}
	}
	return selected_table;
}

template<typename V,
	typename std::enable_if<std::is_floating_point<V>::value>::type* = nullptr>
unsigned int sample_categorical(const V* probability, V sum, unsigned int length)
{
#if !defined(NDEBUG)
	if (length == 0)
		fprintf(stderr, "sample_categorical WARNING: Specified length is zero.\n");
#endif

	/* select the new table assignment */
	V random = sum * ((V) engine() / engine.max());

	V aggregator = 0.0;
	unsigned int selected_table = length - 1;
	for (unsigned int j = 0; j < length; j++) {
		aggregator += probability[j];
		if (random < aggregator) {
			selected_table = j;
			break;
		}
	}
	return selected_table;
}

unsigned int sample_categorical(
	const unsigned int* probability,
	unsigned int sum, unsigned int length)
{
#if !defined(NDEBUG)
	if (length == 0)
		fprintf(stderr, "sample_categorical WARNING: Specified length is zero.\n");
#endif

	/* select the new table assignment */
	unsigned int random = engine() % sum;

	unsigned int aggregator = 0;
	unsigned int selected_table = length - 1;
	for (unsigned int j = 0; j < length; j++) {
		aggregator += probability[j];
		if (random < aggregator) {
			selected_table = j;
			break;
		}
	}
	return selected_table;
}


/**
 * Sampling functions for uniform, beta and Dirichlet distributions.
 */

/* forward declarations */

template<typename T> struct array;

/* returns a sample from the uniform distribution over {0, ..., n - 1} */
inline unsigned int sample_uniform(unsigned int n) {
	return engine() % n;
}

template<typename T>
inline const T& sample_uniform(const T* elements, unsigned int length) {
	return elements[engine() % length];
}

template<typename T>
inline const T& sample_uniform(const array<T>& elements) {
	return sample_uniform(elements.data, (unsigned int) elements.length);
}

template<typename V>
inline V sample_uniform() {
	return (V) engine() / engine.max();
}

template<typename V>
inline bool sample_bernoulli(const V& p) {
	return sample_uniform<V>() < p;
}

template<typename V>
inline V sample_beta(const V& alpha) {
	std::gamma_distribution<V> first_gamma = std::gamma_distribution<V>(1.0);
	std::gamma_distribution<V> second_gamma = std::gamma_distribution<V>(alpha);
	V first = first_gamma(engine);
	V second = second_gamma(engine);
	return first / (first + second);
}

template<typename V>
inline V sample_beta(const V& alpha, const V& beta) {
	std::gamma_distribution<V> first_gamma = std::gamma_distribution<V>(alpha);
	std::gamma_distribution<V> second_gamma = std::gamma_distribution<V>(beta);
	V first = first_gamma(engine);
	V second = second_gamma(engine);
	return first / (first + second);
}

template<typename V>
inline V sample_gamma(const V& alpha, const V& beta) {
	std::gamma_distribution<V> gamma = std::gamma_distribution<V>(alpha, beta);
	return gamma(engine);
}

template<typename V>
inline void sample_dirichlet(V* dst, const V* alpha, unsigned int length) {
	V sum = 0.0;
	for (unsigned int i = 0; i < length; i++) {
		if (alpha[i] == 0.0) {
			dst[i] = 0.0;
		} else {
			std::gamma_distribution<V> gamma_dist(alpha[i], 1.0);
			V value = gamma_dist(engine);
			dst[i] = value;
			sum += value;
		}
	}

	for (unsigned int i = 0; i < length; i++)
		dst[i] /= sum;
}


} /* namespace core */

#endif /* RANDOM_H_ */
