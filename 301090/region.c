#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "region.h"
#include "segment.h"
#include "list.h"
#include "hashset.h"


void destroy_dead_transaction_node(node_t* node) {
    if (node->content) {
        free(node->content);
    }

    free(node);
}

region_t* create_region(size_t size, size_t align) {
    // Allocate region structure
    region_t* region = (region_t*) malloc(sizeof(region_t));
    if (!region) return NULL;

    // Align memory
    if (posix_memalign(&(region->start), align, size) != 0) {
        free(region);
        return NULL;
    }

    // Init segments
    size_t segments_array_size = size / align;
    region->segments = (segment_t**) malloc(segments_array_size * sizeof(segment_t*));
    if (!region->segments) {
        free(region->start);
        free(region);
        return NULL;
    }

    for (size_t i = 0; i < segments_array_size; i++) {
        (region->segments)[i] = create_segment(align);
        if (!(region->segments)[i]) {
            for (size_t j = 0; j < i; j++) {
                destroy_segment((region->segments)[j]);
            }
            free(region->segments);
            free(region->start);
            free(region);
            return NULL;
        }
    }
    region->n_segments = segments_array_size;

    // Init dead transactions set
    region->dead_transactions = hashset_create();

    // Init shared_memory with 0
    memset(region->start, 0, size);

    // Finish initialization and return region
    atomic_init(&(region->tx_id), 0);
    region->size = size;
    region->align = align;
    return region;
}

void destroy_region(region_t* region) {
    if (region->start) {
        free(region->start);
    }

    // Destroy dead transactions list
    if (region->dead_transactions) {
        hashset_destroy(region->dead_transactions);
    }

    // Destroy segments
    if (region->segments) {
        size_t segments_array_size = region->size / region->align;
        for (size_t i = 0; i < segments_array_size; i++) {
            destroy_segment((region->segments)[i]);
        }
        free(region->segments);
    }
    free(region);
}

uint_t increment_and_fetch_tx_id(region_t* region) {
    uint_t val = atomic_fetch_add(&(region->tx_id), 1);
    return val + 1;
}

size_t get_segment_start_index(region_t* region, const void* address) {
    void* start = region->start;
    size_t align = region->align;
    ptrdiff_t ptrdiff = address - start;

    return ptrdiff / align;
}

size_t get_segment_end_index(region_t* region, const void* address, size_t size) {
    void* start = region->start;
    size_t align = region->align;
    ptrdiff_t ptrdiff = address + size - start;

    return (ptrdiff / align);
}

segment_t* get_segment(region_t* region, size_t index) {
    return (region->segments)[index];
}

void add_dead_transaction(region_t* region, transaction_t* transaction) {
    void* item = (void*) transaction->tx_id;
    hashset_add(region->dead_transactions, item);
}

bool is_transaction_dead(region_t* region, uint_t dead_tx_id) {
    return hashset_is_member(region->dead_transactions, (void*) dead_tx_id);
}