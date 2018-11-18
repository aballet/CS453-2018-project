#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct versioned_wr_lock_inside {
    atomic_uint lock;
    atomic_uint version;
    atomic_uint thread_id;
} versioned_wr_lock_inside_t;

typedef struct versioned_wr_lock {
    _Atomic versioned_wr_lock_inside_t inside_lock;
} versioned_wr_lock_t;

versioned_wr_lock_t* create_lock();
void destroy_lock(versioned_wr_lock_t* lock);
uint32_t get_lock_flag(versioned_wr_lock_t* lock);
uint32_t get_lock_version(versioned_wr_lock_t* lock);
uint32_t get_lock_thread_id(versioned_wr_lock_t* lock);
bool acquire_lock(versioned_wr_lock_t* lock, uint32_t tx_id);
void release_lock(versioned_wr_lock_t* lock, uint32_t new_version);
