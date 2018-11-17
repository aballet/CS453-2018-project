#include <stdlib.h>
#include <stdatomic.h>
#include "versioned_write_lock.h"




// TODO : make the lock spin!!!!





versioned_wr_lock_t* lock_create() {
    versioned_wr_lock_t* lock = (versioned_wr_lock_t*) malloc(sizeof(versioned_wr_lock_t));
    if (!lock) return NULL;
    lock->flag = ATOMIC_FLAG_INIT;
    return lock;
}

void lock_destroy(versioned_wr_lock_t* lock) {
    if (!lock) return;
    free(lock);
}

bool lock_acquire(versioned_wr_lock_t* lock) {
    if (!lock) return false;
    uint32_t f = UNLOCKED;
    return atomic_compare_exchange_weak(&(lock->flag), &f, LOCKED)
}

bool lock_release(versioned_wr_lock_t* lock) {
    if (!lock) return false;
    uint32_t f = LOCKED;
    if (atomic_compare_exchange_weak(&(lock->flag), &f, UNLOCKED)) {
        lock->version += 1;
        return true;
    }
    return false;
}
