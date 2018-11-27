#include "versioned_lock.h"
#include <stdio.h>

versioned_lock_t* create_versioned_lock() {
    versioned_lock_t* lock = (versioned_lock_t*) malloc(sizeof(versioned_lock_t));
    if (!lock) return NULL;
    atomic_init(&(lock->tx_id), 0);
    lock->version = 0;
    return lock;
}

void destroy_versioned_lock(versioned_lock_t* lock) {
    free(lock);
}

uint_t get_versioned_lock_tx_id(versioned_lock_t* lock) {
    return lock->tx_id;
}

uint_t get_versioned_lock_version(versioned_lock_t* lock) {
    return lock->version;
}

bool acquire_versioned_lock(versioned_lock_t* lock, uint_t tx_id) {
    if(lock->tx_id == tx_id) return true;

    uint_t unlocked_tx_id = 0;
    bool acquired = false;
    int i = 100;
    while (i > 0) {
        acquired = atomic_compare_exchange_weak(&(lock->tx_id), &unlocked_tx_id, tx_id);
        if (acquired) return true;
        unlocked_tx_id = 0;

        i--;
    }
    return false;
}

void release_versioned_lock(versioned_lock_t* lock, uint_t tx_id, uint_t new_version) {
    if(lock->tx_id != tx_id) return;

    lock->version = new_version;
    atomic_store(&(lock->tx_id), 0);
}

void release_versioned_lock_untouched(versioned_lock_t* lock, uint_t tx_id) {
    if(lock->tx_id != tx_id) return;

    atomic_store(&(lock->tx_id), 0);
}

void print_versioned_lock(versioned_lock_t* lock) {
    printf("LOCK(tx_id:%u,version:%u)\n", lock->tx_id, lock->version);
}
