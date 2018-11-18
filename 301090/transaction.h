#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "bloom.h"

typedef struct transaction {
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
