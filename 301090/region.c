#include "region.h"

uint_t increment_and_fetch_tx_id(region_t* region) {
    uint_t val = atomic_fetch_add(&(region->tx_id), 1);
    return val + 1;
}

int get_locks_start_index(region_t* region, const void* address) {
    //return 0;
    void* start = region->start;
    size_t align = region->align;
    ptrdiff_t ptrdiff = address - start;

    return ptrdiff / align;
}

int get_locks_end_index(region_t* region, const void* address, size_t size) {
    //return region->size / region->align - 1;
    void* start = region->start;
    size_t align = region->align;
    ptrdiff_t ptrdiff = address + size - start;

    return ptrdiff / align - 1;
}
