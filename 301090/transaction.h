#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "bloom.h"
#include "linked_list.h"
#include "global_counter.h"
#include "versioned_write_lock.h"

// ******* REGION

typedef struct region {
    void* start;
    atomic_uint tx_id;
    global_counter_t* counter;
    versioned_wr_lock_t* lock;
    size_t size;
    size_t align;
} region_t;

uint32_t increment_and_fetch_tx_id(region_t* region);


// ******* TRANSACTION
typedef struct transaction {
    uint32_t id;
    bool is_read_only;
    uint32_t rv;
    uint32_t wv;
    list_t* read_set;
    list_t* write_set;
    struct bloom* write_set_bloom_filter;
} transaction_t;

typedef struct load {
    void* read_address;
} load_t;

typedef struct store {
    void* address_to_be_written;
    void* value_to_be_written;
    size_t size;
} store_t;

transaction_t* create_transaction(region_t* region, bool is_read_only);
void destroy_transaction(transaction_t* transaction);
load_t* new_load();
store_t* new_store(size_t size);
