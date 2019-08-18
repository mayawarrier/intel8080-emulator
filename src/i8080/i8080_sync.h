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
        #define EMU_MUTEX_HANDLE pthread_mutex_t
    #else
        /* Use char, implementation relies on __atomic_load_n
         * and __atomic_store_n */
        #define EMU_MUTEX_HANDLE char
    #endif
#elif defined _MSC_VER
    // Rely on Windows to provide a mutex
    #include <windows.h>
    #define EMU_MUTEX_HANDLE HANDLE*
#else
    /* A volatile char * ~should~ provide the best chance
     * at proper sync, if nothing else is available. */
    #define EMU_MUTEX_HANDLE volatile char *
#endif

// Initialize a mutex.
void inline emu_mutex_init(EMU_MUTEX_HANDLE handle);
// Waits until the mutex is released, then locks it and takes over control.
void inline emu_mutex_lock(EMU_MUTEX_HANDLE handle);
// Unlocks/releases a mutex.
void inline emu_mutex_unlock(EMU_MUTEX_HANDLE handle);
// Destroys a mutex.
void inline emu_mutex_destroy(EMU_MUTEX_HANDLE handle);

// -------------- Implementation for different compilers/machines -------------------

#if defined __GNUC__

// If pthreads are available, use them instead
#if PTHREADS_AVAILABLE

void inline emu_mutex_init(EMU_MUTEX_HANDLE handle) {
    handle = (EMU_MUTEX_HANDLE)PTHREAD_MUTEX_INITIALIZER;
}

void inline emu_mutex_lock(EMU_MUTEX_HANDLE handle) {
    pthread_mutex_lock(&handle);
}

void inline emu_mutex_unlock(EMU_MUTEX_HANDLE handle) {
    pthread_mutex_unlock(&handle);
}

void inline emu_mutex_destroy(EMU_MUTEX_HANDLE handle) {
    pthread_mutex_destroy(&handle);
}

#else

// These should work where pthreads are not available on any POSIX-compliant system.

void inline emu_mutex_init(EMU_MUTEX_HANDLE handle) {
    // Puts the mutex in unlocked state
    handle = (EMU_MUTEX_HANDLE)0;
}

void inline emu_mutex_lock(EMU_MUTEX_HANDLE handle) {
    // Stay here until the mutex is unlocked
    while(__atomic_load_n(handle, __ATOMIC_ACQUIRE) != (EMU_MUTEX_HANDLE)0);
    // Lock the mutex
    __atomic_store_n(handle, (EMU_MUTEX_HANDLE)1, __ATOMIC_RELEASE);
}

void inline emu_mutex_unlock(EMU_MUTEX_HANDLE handle) {
    __atomic_store_n(handle, (EMU_MUTEX_HANDLE)0, __ATOMIC_RELEASE);
}

void inline emu_mutex_destroy(EMU_MUTEX_HANDLE handle) {
    // do nothing, handle is just a char
    (void)handle;
}
#endif

#elif defined _MSC_VER

// These rely on Windows to provide a mutex handle.

void inline emu_mutex_init(EMU_MUTEX_HANDLE handle) {
    *handle = CreateMutex(NULL, FALSE, NULL);
}

void inline emu_mutex_lock(EMU_MUTEX_HANDLE handle) {
    // Request mutex ownership and wait until control is transferred to this thread
    WaitForSingleObject(*handle, INFINITE);
}

void inline emu_mutex_unlock(EMU_MUTEX_HANDLE handle) {
    // Release mutex from this thread
    ReleaseMutex(*handle);
}

void inline emu_mutex_destroy(EMU_MUTEX_HANDLE handle) {
    CloseHandle(*handle);
}

#else

// Use a volatile char *. This may or may not work.

void inline emu_mutex_init(EMU_MUTEX_HANDLE handle) {
    // init as unlocked
    *handle = 0;
}

void inline emu_mutex_lock(EMU_MUTEX_HANDLE handle) {
    // wait until unlocked
    while (*handle != 0);
    // lock
    *handle = 1;
}

void inline emu_mutex_unlock(EMU_MUTEX_HANDLE handle) {
    *handle = 0;
}

void inline emu_mutex_destroy(EMU_MUTEX_HANDLE handle) {
    // nothing to do
    (void)handle;
}

#endif

#endif /* I8080_SYNC_H */