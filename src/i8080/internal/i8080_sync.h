/*
 * An attempt at providing portable synchronization functions.
 *
 * Use pthreads if available on POSIX. This is not standards-compliant with std::thread,
 * but on systems with pthreads, std::thread is just a wrapper around pthreads:
 * https://stackoverflow.com/questions/37110571/when-to-use-pthread-mutex-t
 *
 * Use CRITICAL_SECTION on Windows. It seems to be implemented similarly in
 * tinycthread: https://github.com/tinycthread/tinycthread
 */

#ifndef I8080_SYNC_H
#define I8080_SYNC_H

#include "i8080_types.h"

 /* i8080_mutex_init() -> Initialize a mutex.
  * i8080_mutex_lock() -> Waits until the mutex is released, then locks it and takes over control.
  * i8080_mutex_unlock() -> Unlocks/releases a mutex.
  * i8080_mutex_destroy() -> Destroys a mutex. */

#include "i8080_predef.h"
#ifdef I8080_WINDOWS_MIN_VER

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

#else

    // I8080_UNIDENTIFIED

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
#include "i8080_predef_undef.h"

#endif /* I8080_SYNC_H */