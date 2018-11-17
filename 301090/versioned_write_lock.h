#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct versioned_wr_lock {
    atomic_flag flag;
    uint32_t version;
} versioned_wr_lock_t;

const uint32_t LOCKED = 1;
const uint32_t UNLOCKED = 0;

versioned_wr_lock_t* lock_create();
void lock_destroy(versioned_wr_lock_t* lock);
bool lock_acquire(versioned_wr_lock_t* lock);
bool lock_release(versioned_wr_lock_t* lock);
