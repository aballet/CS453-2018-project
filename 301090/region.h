#ifndef REGION_H
#define REGION_H

#include <stdlib.h>
#include <stdatomic.h>
#include "segment.h"
#include "own_types.h"

typedef struct region {
    void* start;
    size_t size;
    size_t align;
    atomic_uint tx_id;
    size_t n_segments;
    segment_t** segments;
} region_t;

region_t* create_region(size_t size, size_t align);
void destroy_region(region_t* region);
uint_t increment_and_fetch_tx_id(region_t* region);
int get_segment_start_index(region_t* region, const void* address);
int get_segment_end_index(region_t* region, const void* address, size_t size);

#endif /* REGION_H */
