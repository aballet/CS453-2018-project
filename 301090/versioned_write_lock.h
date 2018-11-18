#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct versioned_wr_lock {
    atomic_uint_fast32_t lock;
} versioned_wr_lock_t;

const uint32_t LOCKED = 1;
const uint32_t UNLOCKED = 0;

versioned_wr_lock_t* create_lock();
void destroy_lock(versioned_wr_lock_t* lock);
bool acquire_lock(versioned_wr_lock_t* lock);
bool release_lock(versioned_wr_lock_t* lock);
