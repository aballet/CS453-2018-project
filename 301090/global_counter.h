#ifndef _GLOBAL_COUNTER_H
#define _GLOBAL_COUNTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct global_counter {
    atomic_uint_fast32_t version;
} global_counter_t;

global_counter_t* create_global_counter();
//void destroy_global_counter(global_counter_t* counter);
uint32_t increment_and_fetch_global_counter(global_counter_t* counter);
uint32_t fetch_global_counter(global_counter_t* counter);

#endif
