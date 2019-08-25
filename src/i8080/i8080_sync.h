/*
 * File:   i8080_sync.h
 * Author: dhruvwarrier
 *
 * Attempt to provide portable synchronization fns.
 *
 * Created on August 6, 2019, 11:58 PM
 */

#ifndef I8080_SYNC_H
#define I8080_SYNC_H

 // Define the mutex handle type.
#if defined __GNUC__
	#if EMU_PTHREADS_AVAILABLE
	// Use pthreads, if available
	#include <pthread.h>
	typedef pthread_mutex_t emu_mutex_t;
#else
	/* Use char. Implementation relies on __atomic_load_n
	 * and __atomic_store_n */
	typedef char emu_mutex_t;
#endif
#elif defined _MSC_VER
	// Define minimum supported windows version
	#ifndef WINVER
		#define WINVER 0x0403
	#endif
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0403
	#endif
	// Reduce the size of windows header files
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	/* Critical sections have better performance than
	* standard windows mutexes and appear to be compatible
	* with std::thread. */
	#include <Windows.h>
	typedef CRITICAL_SECTION emu_mutex_t;
#else
	/* A volatile char ~should~ provide the best chance
	 * at proper sync, if nothing else is available. */
	typedef volatile char emu_mutex_t;
#endif

// Initialize a mutex.
inline void emu_mutex_init(emu_mutex_t * handle);
// Waits until the mutex is released, then locks it and takes over control.
inline void emu_mutex_lock(emu_mutex_t * handle);
// Unlocks/releases a mutex.
inline void emu_mutex_unlock(emu_mutex_t * handle);
// Destroys a mutex.
inline void emu_mutex_destroy(emu_mutex_t * handle);

// -------------- Implementation for different compilers/machines -------------------

#if defined __GNUC__

// If pthreads are available, use them instead
#if EMU_PTHREADS_AVAILABLE

inline void emu_mutex_init(emu_mutex_t * handle) {
	pthread_mutex_init(handle, NULL);
}

inline void emu_mutex_lock(emu_mutex_t * handle) {
	pthread_mutex_lock(handle);
}

inline void emu_mutex_unlock(emu_mutex_t * handle) {
	pthread_mutex_unlock(handle);
}

inline void emu_mutex_destroy(emu_mutex_t * handle) {
	pthread_mutex_destroy(handle);
}

#else

// These should work where pthreads are not available on any POSIX-compliant system.

inline void emu_mutex_init(emu_mutex_t * handle) {
	// Puts the mutex in unlocked state
	*handle = (emu_mutex_t)0;
}

inline void emu_mutex_lock(emu_mutex_t * handle) {
	// Stay here until the mutex is unlocked
	while (__atomic_load_n(handle, __ATOMIC_ACQUIRE) != (emu_mutex_t)0);
	// Lock the mutex
	__atomic_store_n(handle, (emu_mutex_t)1, __ATOMIC_RELEASE);
}

inline void emu_mutex_unlock(emu_mutex_t * handle) {
	__atomic_store_n(handle, (emu_mutex_t)0, __ATOMIC_RELEASE);
}

inline void emu_mutex_destroy(emu_mutex_t * handle) {
	// do nothing, handle is just a char *
	(void)handle;
}
#endif

#elif defined _MSC_VER

// These use Windows critical sections for faster mutexes.

inline void emu_mutex_init(emu_mutex_t * handle) {
	/* Initializing with a spin count can hugely improve performance,
	 * by avoiding a kernel call for short critical sections:
	 * https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount */
	InitializeCriticalSectionAndSpinCount(handle, 4000);
}

inline void emu_mutex_lock(emu_mutex_t * handle) {
	EnterCriticalSection(handle);
}

inline void emu_mutex_unlock(emu_mutex_t * handle) {
	LeaveCriticalSection(handle);
}

inline void emu_mutex_destroy(emu_mutex_t * handle) {
	DeleteCriticalSection(handle);
}

#else

// Use a volatile char *. This may or may not work.

inline void emu_mutex_init(emu_mutex_t * handle) {
	// init as unlocked
	*handle = 0;
}

inline void emu_mutex_lock(emu_mutex_t * handle) {
	// wait until unlocked
	while (*handle != 0);
	// lock
	*handle = 1;
}

inline void emu_mutex_unlock(emu_mutex_t * handle) {
	*handle = 0;
}

inline void emu_mutex_destroy(emu_mutex_t * handle) {
	// nothing to do
	(void)handle;
}

#endif

#endif /* I8080_SYNC_H */