#ifndef VERSIONED_LOCK_H
#define VERSIONED_LOCK_H

#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "own_types.h"

typedef struct versioned_lock {
  atomic_uint tx_id;
  uint_t version;
} versioned_lock_t;

versioned_lock_t* create_versioned_lock();
void destroy_versioned_lock(versioned_lock_t* lock);
uint_t get_versioned_lock_tx_id(versioned_lock_t* lock);
uint_t get_versioned_lock_version(versioned_lock_t* lock);
bool acquire_versioned_lock(versioned_lock_t* lock, uint_t tx_id);
void release_versioned_lock(versioned_lock_t* lock, uint_t new_version);

#endif /* VERSIONED_LOCK_H */
