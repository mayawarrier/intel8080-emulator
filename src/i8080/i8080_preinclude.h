/* 
 * File:   i8080_preinclude.h
 * Author: dhruvwarrier
 * 
 * Predefs and platform-specific includes. 
 * Enables build on non-POSIX-compliant compilers (MSVC).
 * 
 * If building on Windows, include this before including <stdio.h> in every source file.
 *
 * The following resources were useful:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 * https://sourceforge.net/p/predef/wiki/Standards/
 * http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 *
 * Created on June 30, 2019, 2:56 PM
 */

#ifndef I8080_BUILD_H
#define I8080_BUILD_H

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(_MSC_VER)
	#define I8080_WINDOWS
#elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
	#define I8080_UNIX
#else
	#define I8080_UNIDENTIFIED
#endif

// Detect minimum supported version for Windows.
#ifdef I8080_WINDOWS
	// Suppress deprecation errors with scanf, printf, etc
	#define _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_DEPRECATE
	#include <Windows.h>
	/* Versions from Windows XP and above support critical sections with spin counts:
	/* https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount */
	#if (WINVER >= 0x0403) || (_WIN32_WINNT >= 0x0403)
		#define I8080_WINDOWS_MIN_VER
	#endif
	#undef _CRT_SECURE_NO_WARNINGS
	#undef _CRT_SECURE_NO_DEPRECATE
#endif

// Detect minimum supported version for POSIX.
#ifdef I8080_UNIX
	#include <unistd.h>
	// Pthreads first appeared in the POSIX standard in IEEE 1003.1c-1995, published in 06/1995:
	// https://standards.ieee.org/standard/1003_1c-1995.html
	#if defined(_POSIX_VERSION) && (_POSIX_VERSION >= 199506L)
		#define I8080_POSIX_MIN_VER
	#endif
#endif

/* If being compiled with GNUC and mutex primitives are not available, GNUC provides
 * atomic synchronization functions from version 4.7.0, that can be used to simulate mutexes. */
#if (defined(__GNUC__) || defined(__GNUG__)) && ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 7) && (__GNUC_PATCHLEVEL__ >= 0))
	#define I8080_GNUC_MIN_VER
#else

/* An attempt at providing portable synchronization functions.
 *
 * Use pthreads if available on POSIX. This is not standards-compliant with std::thread,
 * but on systems with pthreads, std::thread is just a wrapper around pthreads:
 * https://stackoverflow.com/questions/37110571/when-to-use-pthread-mutex-t
 *
 * Use CRITICAL_SECTION on Windows. It seems to be implemented similarly in
 * tinycthread: https://github.com/tinycthread/tinycthread
 */

 // Initialize a mutex.
inline void i8080_mutex_init(i8080_mutex_t * handle);
// Waits until the mutex is released, then locks it and takes over control.
inline void i8080_mutex_lock(i8080_mutex_t * handle);
// Unlocks/releases a mutex.
inline void i8080_mutex_unlock(i8080_mutex_t * handle);
// Destroys a mutex.
inline void i8080_mutex_destroy(i8080_mutex_t * handle);

#ifdef I8080_WINDOWS_MIN_VER
	
	/* Critical sections have better performance than
	 * standard windows mutexes and appear to be compatible
	 * with std::thread. */
	typedef CRITICAL_SECTION i8080_mutex_t;

	inline void i8080_mutex_init(i8080_mutex_t * handle) {
		/* Initializing with a spin count can hugely improve performance,
		 * by avoiding a kernel call for short critical sections:
		 * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount */
		InitializeCriticalSectionAndSpinCount(handle, 4000);
	}

	inline void i8080_mutex_lock(i8080_mutex_t * handle) {
		EnterCriticalSection(handle);
	}

	inline void i8080_mutex_unlock(i8080_mutex_t * handle) {
		LeaveCriticalSection(handle);
	}

	inline void i8080_mutex_destroy(i8080_mutex_t * handle) {
		DeleteCriticalSection(handle);
	}

#elif defined(I8080_POSIX_MIN_VER)

	// Use pthreads, since they are available
	#include <pthread.h>
	typedef pthread_mutex_t i8080_mutex_t;

	inline void i8080_mutex_init(i8080_mutex_t * handle) {
		pthread_mutex_init(handle, NULL);
	}

	inline void i8080_mutex_lock(i8080_mutex_t * handle) {
		pthread_mutex_lock(handle);
	}

	inline void i8080_mutex_unlock(i8080_mutex_t * handle) {
		pthread_mutex_unlock(handle);
	}

	inline void i8080_mutex_destroy(i8080_mutex_t * handle) {
		pthread_mutex_destroy(handle);
	}

#elif defined(I8080_GNUC_MIN_VER)
	
	// Use char instead of _Bool to guarantee that it's byte-sized.
	typedef char i8080_mutex_t;

	// Simulate a mutex using acquire-release semantics.

	inline void i8080_mutex_init(i8080_mutex_t * handle) {
		// Puts the mutex in unlocked state
		*handle = (i8080_mutex_t)0;
	}

	inline void i8080_mutex_lock(i8080_mutex_t * handle) {
		// Stay here until the mutex is unlocked
		while (__atomic_load_n(handle, __ATOMIC_ACQUIRE) != (i8080_mutex_t)0);
		// Lock the mutex
		__atomic_store_n(handle, (i8080_mutex_t)1, __ATOMIC_RELEASE);
	}

	inline void i8080_mutex_unlock(i8080_mutex_t * handle) {
		__atomic_store_n(handle, (i8080_mutex_t)0, __ATOMIC_RELEASE);
	}

	inline void i8080_mutex_destroy(i8080_mutex_t * handle) {
		(void)handle;
	}

#elif defined(I8080_UNIDENTIFIED)

	/* A volatile char ~should~ provide the best chance
	 * at proper sync, if nothing else is available. */

	inline void i8080_mutex_init(i8080_mutex_t * handle) {
		// init as unlocked
		*handle = 0;
	}

	inline void i8080_mutex_lock(i8080_mutex_t * handle) {
		// wait until unlocked
		while (*handle != 0);
		// lock
		*handle = 1;
	}

	inline void i8080_mutex_unlock(i8080_mutex_t * handle) {
		*handle = 0;
	}

	inline void i8080_mutex_destroy(i8080_mutex_t * handle) {
		// nothing to do
		(void)handle;
	}

#endif