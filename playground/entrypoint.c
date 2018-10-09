#include "config.h"
#if !defined(CONFIG_USE_CPP)
// ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――

// External headers
#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

// Internal headers
#include "entrypoint.h"
#include "runner.h"

// ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――

/** Lock initialization.
 * @param lock Uninitialized lock structure
 * @return Whether initialization is a success
**/
bool lock_init(struct lock_t* lock __attribute__((unused))) {
    lock->flag = false;
    return true;
}

/** Lock clean up.
 * @param lock Initialized lock structure
**/
void lock_cleanup(struct lock_t* lock __attribute__((unused))) {
    lock->flag = false;
}

/** [thread-safe] Acquire the given lock, wait if it has already been acquired.
 * @param lock Initialized lock structure
**/
void lock_acquire(struct lock_t* lock __attribute__((unused))) {
    bool finished = false;
    while(!finished) {
      bool b = false;
      if (atomic_compare_exchange_weak(&(lock->flag), &b, true)) {
        finished = true;
      }
    };
}

/** [thread-safe] Release the given lock.
 * @param lock Initialized lock structure
**/
void lock_release(struct lock_t* lock __attribute__((unused))) {
    bool b = true;
    atomic_compare_exchange_weak(&(lock->flag), &b, false);
}

// ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――

/** Thread entry point.
 * @param nb   Total number of threads
 * @param id   This thread ID (from 0 to nb-1 included)
 * @param lock Instance of your lock
**/
void entry_point(size_t nb, size_t id, struct lock_t* lock) {
    printf("Hello from C version in thread %lu/%lu\n", id, nb); // Feel free to remove me
    for (int i = 0; i < 10000; ++i) {
        lock_acquire(lock);
        shared_access();
        lock_release(lock);
    }
}

// ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――
#endif
