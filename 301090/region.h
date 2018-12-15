#ifndef REGION_H
#define REGION_H


#include "own_types.h"

region_t* create_region(size_t size, size_t align);
void destroy_region(region_t* region);
uint_t increment_and_fetch_tx_id(region_t* region);
size_t get_segment_start_index(region_t* region, const void* address);
size_t get_segment_end_index(region_t* region, const void* address, size_t size);
segment_t* get_segment(region_t* region, size_t index);


#endif /* REGION_H */
