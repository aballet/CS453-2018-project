typedef struct region {
    void* start;
    global_counter_t* counter;
    versioned_wr_lock_t* lock;
    size_t size;
    size_t align;
} region_t;
