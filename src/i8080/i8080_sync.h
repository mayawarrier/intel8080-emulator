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
    #if PTHREADS_AVAILABLE
        // Use pthreads, if available
        #include <pthread.h>
        typedef pthread_mutex_t emu_mutex_t;
    #else
        /* Use char, implementation relies on __atomic_load_n
         * and __atomic_store_n */
        typedef char emu_mutex_t;
    #endif
#elif defined _MSC_VER
    // Rely on Windows to provide a mutex
    #include <windows.h>
    typedef HANDLE emu_mutex_t;
#else
    /* A volatile char ~should~ provide the best chance
     * at proper sync, if nothing else is available. */
    typedef volatile char emu_mutex_t;
#endif

// Initialize a mutex.
static inline void emu_mutex_init(emu_mutex_t * handle);
// Waits until the mutex is released, then locks it and takes over control.
static inline void emu_mutex_lock(emu_mutex_t * handle);
// Unlocks/releases a mutex.
static inline void emu_mutex_unlock(emu_mutex_t * handle);
// Destroys a mutex.
static inline void emu_mutex_destroy(emu_mutex_t * handle);

// -------------- Implementation for different compilers/machines -------------------

#if defined __GNUC__

// If pthreads are available, use them instead
#if PTHREADS_AVAILABLE

static inline void emu_mutex_init(emu_mutex_t * handle) {
    pthread_mutex_init(handle, NULL);
}

static inline void emu_mutex_lock(emu_mutex_t * handle) {
    pthread_mutex_lock(handle);
}

static inline void emu_mutex_unlock(emu_mutex_t * handle) {
    pthread_mutex_unlock(handle);
}

static inline void emu_mutex_destroy(emu_mutex_t * handle) {
    pthread_mutex_destroy(handle);
}

#else

// These should work where pthreads are not available on any POSIX-compliant system.

static inline void emu_mutex_init(emu_mutex_t * handle) {
    // Puts the mutex in unlocked state
    *handle = (emu_mutex_t)0;
}

static inline void emu_mutex_lock(emu_mutex_t * handle) {
    // Stay here until the mutex is unlocked
    while(__atomic_load_n(handle, __ATOMIC_ACQUIRE) != (emu_mutex_t)0);
    // Lock the mutex
    __atomic_store_n(handle, (emu_mutex_t)1, __ATOMIC_RELEASE);
}

static inline void emu_mutex_unlock(emu_mutex_t * handle) {
    __atomic_store_n(handle, (emu_mutex_t)0, __ATOMIC_RELEASE);
}

static inline void emu_mutex_destroy(emu_mutex_t * handle) {
    // do nothing, handle is just a char *
    (void)handle;
}
#endif

#elif defined _MSC_VER

// These rely on Windows to provide a mutex handle.

static inline void emu_mutex_init(emu_mutex_t * handle) {
    *handle = CreateMutex(NULL, FALSE, NULL);
}

static inline void emu_mutex_lock(emu_mutex_t * handle) {
    // Request mutex ownership and wait until control is transferred to this thread
    WaitForSingleObject(*handle, INFINITE);
}

static inline void emu_mutex_unlock(emu_mutex_t * handle) {
    // Release mutex from this thread
    ReleaseMutex(*handle);
}

static inline void emu_mutex_destroy(emu_mutex_t * handle) {
    CloseHandle(*handle);
}

#else

// Use a volatile char *. This may or may not work.

static inline void emu_mutex_init(emu_mutex_t * handle) {
    // init as unlocked
    *handle = 0;
}

static inline void emu_mutex_lock(emu_mutex_t * handle) {
    // wait until unlocked
    while (*handle != 0);
    // lock
    *handle = 1;
}

static inline void emu_mutex_unlock(emu_mutex_t * handle) {
    *handle = 0;
}

static inline void emu_mutex_destroy(emu_mutex_t * handle) {
    // nothing to do
    (void)handle;
}

#endif

#endif /* I8080_SYNC_H */