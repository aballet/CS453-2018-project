#include <stddef.h>
#include <stdint.h>

typedef struct global_counter {
    atomic_uint version;
} global_counter_t;

global_counter_t* global_counter_create();
uint32_t increment_and_fetch(global_counter_t* counter);
uint32_t fetch(global_counter_t* counter);
