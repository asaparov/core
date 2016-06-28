#ifndef TIMER_H_
#define TIMER_H_

#if defined(_WIN32)
#include <Windows.h>
#else
#include <time.h>
#endif

struct timer
{
#if defined(_WIN32)
	ULONGLONG start_time, end_time;
#else
	timespec start_time, end_time;
#endif

	timer() {
		start();
	}

	inline void start() {
#if defined(_WIN32)
		start_time = GetTickCount64();
#else
		clock_gettime(CLOCK_REALTIME, &start_time);
#endif
	}

	inline uint64_t milliseconds() {
#if defined(_WIN32)
		end_time = GetTickCount64();
		return (uint64_t) (end_time - start_time);
#else
		clock_gettime(CLOCK_REALTIME, &end_time);
		return (uint64_t) (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
#endif
	}

	inline double nanoseconds() {
#if defined(_WIN32)
		end_time = GetTickCount64();
		return (end_time - start_time) * 1.0e6;
#else
		clock_gettime(CLOCK_REALTIME, &end_time);
		return (end_time.tv_sec - start_time.tv_sec) * 1.0e9 + (end_time.tv_nsec - start_time.tv_nsec);
#endif
	}
};

inline uint64_t milliseconds() {
#if defined(_WIN32)
	return GetTickCount64();
#else
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return (uint64_t) time.tv_sec * 1000 + time.tv_nsec / 1000000;
#endif
}

#endif /* TIMER_H_ */
