#ifndef LOCK_H
#define LOCK_H

#include "own_types.h"

lock_t* create_lock();
void destroy_lock(lock_t* lock);
uint_t get_lock_tx_id(lock_t* lock);
bool acquire_lock(transaction_t* transaction, lock_t* lock);
bool release_lock(transaction_t* transaction, lock_t* lock);


#endif /* LOCK_H */
