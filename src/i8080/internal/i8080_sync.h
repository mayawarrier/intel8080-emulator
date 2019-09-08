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

extern void i8080_mutex_init(i8080_mutex_t * handle);
extern void i8080_mutex_lock(i8080_mutex_t * handle);
extern void i8080_mutex_unlock(i8080_mutex_t * handle);
extern void i8080_mutex_destroy(i8080_mutex_t * handle);

#endif /* I8080_SYNC_H */