#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdlib.h>
#include <stdbool.h>
#include "own_types.h"
#include "region.h"
#include "list.h"

typedef struct transaction {
    uint_t tx_id;
    bool is_read_only;
    uint_t rv;
    uint_t wv;
    list_t* read_set;
    list_t* write_set;
    //struct bloom* write_set_bloom_filter;
} transaction_t;

typedef struct load {
    void const* read_address;
    size_t size;
} load_t;

typedef struct store {
    void* address_to_be_written;
    void* value_to_be_written;
    size_t size;
} store_t;

transaction_t* create_transaction(region_t* region, bool is_read_only);
void destroy_transaction(transaction_t* transaction);
load_t* new_load(size_t size);
store_t* new_store(size_t size);

#endif /* TRANSACTION_H */
