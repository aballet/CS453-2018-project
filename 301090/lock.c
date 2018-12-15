#include <stdlib.h>

#include "lock.h"

lock_t* create_lock() {
    lock_t* lock = (lock_t*) malloc(sizeof(lock_t));
    if (!lock) return NULL;
    atomic_init(&(lock->tx_id), 0);
    return lock;
}

void destroy_lock(lock_t* lock) {
    free(lock);
}

uint_t get_lock_tx_id(lock_t* lock) {
    return lock->tx_id;
}

bool acquire_lock(transaction_t* transaction, lock_t* lock) {
    if(lock->tx_id == transaction->tx_id) return true;

    uint_t unlocked_tx_id = 0;
    bool acquired = false;
    //int i = 100;
    //while (i > 0) {
    while (!acquired) {
        acquired = atomic_compare_exchange_weak(&(lock->tx_id), &unlocked_tx_id, transaction->tx_id);
        //if (acquired) return true;
        unlocked_tx_id = 0;

        //i--;
    }
    return true;
}

bool release_lock(transaction_t* transaction, lock_t* lock) {
    if(lock->tx_id != transaction->tx_id) return false;

    atomic_store(&(lock->tx_id), 0);
    return true;
}
