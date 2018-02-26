/**
 * \file timer.h
 *
 * <!-- Author: asaparov -->
 */

#ifndef TIMER_H_
#define TIMER_H_

#if defined(_WIN32)
#define _WINSOCKAPI_
#include <Windows.h>
#undef max
#undef min
#else
#include <time.h>
#endif


namespace core {


/**
 * A simple structure that measures time.
 */
struct timer
{
#if defined(_WIN32)
	ULONGLONG start_time, end_time;
#else
	timespec start_time, end_time;
#endif

	/**
	 * Constructor that starts the timer.
	 */
	timer() {
		start();
	}

	/**
	 * Starts the timer.
	 */
	inline void start() {
#if defined(_WIN32)
		start_time = GetTickCount64();
#else
		clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
	}

	/**
	 * Returns the number of milliseconds elapsed since the timer was last started.
	 */
	inline unsigned long long milliseconds() {
#if defined(_WIN32)
		end_time = GetTickCount64();
		return (unsigned long long) (end_time - start_time);
#else
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		return (unsigned long long) (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
#endif
	}

	/**
	 * Returns the number of nanoseconds elapsed since the timer was last started.
	 */
	inline double nanoseconds() {
#if defined(_WIN32)
		end_time = GetTickCount64();
		return (end_time - start_time) * 1.0e6;
#else
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		return (end_time.tv_sec - start_time.tv_sec) * 1.0e9 + (end_time.tv_nsec - start_time.tv_nsec);
#endif
	}
};

/**
 * On Windows, this returns the number of milliseconds elapsed since the system
 * was started. On other platforms, it returns the system's best estimate of
 * the current time of day.
 */
inline unsigned long long milliseconds() {
#if defined(_WIN32)
	return GetTickCount64();
#else
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return (unsigned long long) time.tv_sec * 1000 + time.tv_nsec / 1000000;
#endif
}


} /* namespace core */

#endif /* TIMER_H_ */
