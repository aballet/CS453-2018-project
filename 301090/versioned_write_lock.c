#include <stdlib.h>
#include <stdatomic.h>
#include "versioned_write_lock.h"

versioned_wr_lock_t* create_lock() {
    versioned_wr_lock_t* lock = (versioned_wr_lock_t*) malloc(sizeof(versioned_wr_lock_t));
    if (!lock) return NULL;
    lock->flag = ATOMIC_FLAG_INIT;
    lock->version = 0;
    return lock;
}

void destroy_lock(versioned_wr_lock_t* lock) {
}

uint32_t get_lock_flag(uint32_t lock) {
    return lock >> 31;
}

uint32_t set_lock_flag(uint32_t lock, bool locked) {
    if (locked) {
        return lock | 0x80000000;
    } else {
        return get_lock_version(lock);
    }
}

uint32_t get_lock_version(uint32_t lock) {
    return lock & 0x7fffffff;
}

uint32_t set_lock_version(uint32_t lock, uint32_t version) {
    return (lock & 0x80000000) | version;
}

uint32_t fetch_lock(versioned_wr_lock_t* lock) {
    return atomic_load(&(lock->lock));
}

bool acquire_lock(versioned_wr_lock_t* lock) {
    uint32_t tmp_lock = (uint32_t) lock->lock;
    uint32_t lock_locked = set_lock_flag(tmp_lock, true)
    uint32_t lock_unlocked = set_lock_flag(tmp_lock, false)

    bool acquired = false;
    int i = 1000;
    while (!acquired || i <= 0) {
        atomic_compare_exchange_weak(&(lock->lock), &lock_unlocked, lock_locked);
        lock_unlocked = set_lock_flag(tmp_lock, false);
        i--;
    }
}

void release_lock(versioned_wr_lock_t* lock, uint32_t new_version) {
    uint32_t tmp_lock = (uint32_t) lock->lock;
    atomic_store(&(lock->lock), set_lock_version(tmp_lock, new_version);
}
