#include <stdlib.h>
#include "versioned_write_lock.h"

versioned_wr_lock_t* create_lock() {
    versioned_wr_lock_t* lock = (versioned_wr_lock_t*) malloc(sizeof(versioned_wr_lock_t));
    if (!lock) return NULL;
    versioned_wr_lock_inside_t inside_lock = lock->inside_lock;
    inside_lock.lock = 0;
    inside_lock.version = 0;
    inside_lock.thread_id = 0;
    lock->inside_lock = inside_lock;
    return lock;
}

void destroy_lock(versioned_wr_lock_t* lock) {
}

uint32_t get_lock_flag(versioned_wr_lock_t* lock) {
    versioned_wr_lock_inside_t inside_lock = lock->inside_lock;
    return inside_lock.lock;
}

uint32_t get_lock_version(versioned_wr_lock_t* lock) {
    versioned_wr_lock_inside_t inside_lock = lock->inside_lock;
    return inside_lock.version;
}

uint32_t get_lock_thread_id(versioned_wr_lock_t* lock) {
    versioned_wr_lock_inside_t inside_lock = lock->inside_lock;
    return inside_lock.thread_id;
}

bool acquire_lock(versioned_wr_lock_t* lock, uint32_t tx_id) {
    versioned_wr_lock_inside_t actual_lock = lock->inside_lock;
    versioned_wr_lock_inside_t lock_locked = actual_lock;
    lock_locked.lock = 1;
    lock_locked.thread_id = tx_id;
    versioned_wr_lock_inside_t lock_unlocked = actual_lock;
    lock_unlocked.lock = 0;

    bool acquired = false;
    int i = 100;
    while (i > 0) {
        acquired = atomic_compare_exchange_weak(&(lock->inside_lock), &lock_unlocked, lock_locked);
        if (acquired) return true;
        if (lock_unlocked.thread_id == tx_id) return true;
        lock_unlocked.lock = 0;
        i--;
    }
    return false;
}

void release_lock(versioned_wr_lock_t* lock, uint32_t new_version) {
    versioned_wr_lock_inside_t actual_lock = lock->inside_lock;
    actual_lock.lock = 0;
    actual_lock.version = new_version;
    atomic_store(&(lock->inside_lock), actual_lock);
}
