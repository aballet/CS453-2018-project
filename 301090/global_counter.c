#include <stdlib.h>
#include <stdatomic.h>
#include "global_counter.h"


global_counter_t* global_counter_create() {
    global_counter_t* counter = (global_counter_t*) malloc(sizeof(global_counter_t));
    if (!counter) return NULL;
    atomic_init(&(counter->version), 0);
    return counter;
}

uint32_t increment_and_fetch_global_counter(global_counter_t* counter) {
    if (!lock) return false;
    uint32_t val = atomic_fetch_add(&(counter->version), 1);
    return val + 1;
}

uint32_t fetch_global_counter(global_counter_t* counter) {
    return atomic_load(&(counter->version));
}
