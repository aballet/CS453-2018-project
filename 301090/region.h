#ifndef REGION_H
#define REGION_H

#include <stdlib.h>
#include <stdatomic.h>
#include "global_counter.h"
#include "versioned_lock.h"
#include "own_types.h"

typedef struct region {
    void* start;
    atomic_uint tx_id;
    global_counter_t* counter;
    versioned_lock_t** locks;
    size_t size;
    size_t align;
} region_t;

uint_t increment_and_fetch_tx_id(region_t* region);
int get_locks_start_index(region_t* region, const void* address);
int get_locks_end_index(region_t* region, const void* address, size_t size);

#endif /* REGION_H */
