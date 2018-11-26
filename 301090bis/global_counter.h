#ifndef GLOBAL_COUNTER_H
#define GLOBAL_COUNTER_H

#include <stdlib.h>
#include <stdatomic.h>
#include "own_types.h"

typedef struct global_counter {
    atomic_uint version;
} global_counter_t;

global_counter_t* create_global_counter();
void destroy_global_counter(global_counter_t* counter);
uint_t increment_and_fetch_global_counter(global_counter_t* counter);
uint_t fetch_global_counter(global_counter_t* counter);

#endif /* GLOBAL_COUNTER_H */
