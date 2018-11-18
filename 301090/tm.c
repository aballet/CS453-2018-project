/**
 * @file   tm.c
 * @author Adrien Ballet
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <stdlib.h>
#include <pthread.h>
// #include "double_linked_list.h"

// Internal headers
#include <tm.h>

// -------------------------------------------------------------------------- //

/** Define a proposition as likely true.
 * @param prop Proposition
**/
#undef likely
#ifdef __GNUC__
    #define likely(prop) \
        __builtin_expect((prop) ? 1 : 0, 1)
#else
    #define likely(prop) \
        (prop)
#endif

/** Define a proposition as likely false.
 * @param prop Proposition
**/
#undef unlikely
#ifdef __GNUC__
    #define unlikely(prop) \
        __builtin_expect((prop) ? 1 : 0, 0)
#else
    #define unlikely(prop) \
        (prop)
#endif

/** Define one or several attributes.
 * @param type... Attribute names
**/
#undef as
#ifdef __GNUC__
    #define as(type...) \
        __attribute__((type))
#else
    #define as(type...)
    #warning This compiler has no support for GCC attributes
#endif

// -------------------------------------------------------------------------- //
// Lock helper functions

int init_lock(region_t* region) {
    if (!region) {
        return -1;
    }
    pthread_rwlock_t* rwlock = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t));
    if (!rwlock) {
        return -1;
    }
    if(pthread_rwlock_init(rwlock, NULL) == 0) {
        region->lock = rwlock;
        return 0;
    } else {
        free(rwlock);
        return -1;
    }
}

int destroy_lock(region_t* region) {
    if (!region) {
        return -1;
    }
    if (!region->lock) {
        return -1;
    }
    if(pthread_rwlock_destroy(region->lock) == 0) {
        free(region->lock);
        region->lock = NULL;
        return 0;
    } else {
        return -1;
    }
}

bool acquire_read_lock(region_t* region) {
    return pthread_rwlock_rdlock(region->lock) == 0;
}

bool acquire_write_lock(region_t* region) {
    return pthread_rwlock_wrlock(region->lock) == 0;
}

void release_read_lock(region_t* region) {
    pthread_rwlock_unlock(region->lock);
}

void release_write_lock(region_t* region) {
    pthread_rwlock_unlock(region->lock);
}

// -------------------------------------------------------------------------- //

/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size as(unused), size_t align as(unused)) {
    // Allocate region structure
    region_t* region = (region_t*) malloc(sizeof(region_t));
    if (!region) {
        return invalid_shared;
    }
    // Allocate shared memory
    if (posix_memalign(&(region->start), align, size) != 0) {
        free(region);
        return invalid_shared;
    }
    // Init counter
    region->counter = global_counter_create();
    if (!region->counter) {
        free(region->start);
        free(region);
        return invalid_shared;
    }
    // Init lock
    region->lock = create_lock();
    if (!region->lock) {
        free(region->counter);
        free(region->start);
        free(region);
        return invalid_shared;
    }
    // Init shared_memory with 0
    memset(region->start, 0, size);
    // Finish initialization and return region
    region->size = size;
    region->align = align;
    return region;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared as(unused)) {
    region_t* region = (region_t*) shared;
    if (!region) {
        if (!region->start) {
            free(region->start);
            region->start = NULL;
        }
        // Destroy lock
        if (!region->lock) {
            destroy_lock(region->lock);
            free(lock);

        }
        free(region);
    }
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared as(unused)) {
    region_t* region = (region_t*) shared;
    return region->start;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared as(unused)) {
    region_t* region = (region_t*) shared;
    return region->size;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared as(unused)) {
    region_t* region = (region_t*) shared;
    return region->align;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared as(unused), bool is_ro as(unused)) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = create_transaction(is_ro);
    if (!transaction) return invalid_tx;

    // Sample global version-clock
    transaction->rv = fetch_global_counter(region->counter);

    return transaction;
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;

    if (!transaction->is_read_only) {
        // Lock write_set

        // TODO : Change this part to support mutiple memory locations

        // list_t* write_set = transaction->write_set;
        // node_t* node = write_set->first;
        // while (node) {
        //     // Lock node
        //     node = node->next;
        // }
        acquired = acquire_lock(region->lock);
        if (!acquired) return false;

        // Increment global version-clock
        transaction->wv = increment_and_fetch_global_counter(region->counter);

        // Validate read-set
        uint32_t lock = fetch_lock(region->lock);
        if (get_lock_version(lock) > transaction->rv || get_lock_flag(lock) == LOCKED) {
            return false;
        }

        // Commit and release the locks
        list_t* write_set = transaction->write_set;
        node_t* node = write_set->last;
        while (node) {
            store_t* store = (store_t*) node->content;
            memcpy(store->address_to_be_written, store->value_to_be_written, store->size);
            node = node->previous;
        }

        release_lock(region->lock, transaction->wv);
    }
    return true;
}
/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;

    // Check if load read_address already appears in the write_set
    if (!transaction->is_read_only) {
        // No write_set to check if transaction is read-only
        int found = bloom_check(transaction->write_set_bloom_filter, source, sizeof(void*));
        if (found) {
            list_t* write_set = transaction->write_set;
            node_t* node = write_set->first;
            while (node) {
                store_t* store = (store_t*) node->content;
                if (store->address_to_be_written == source) {
                    // Return value found
                    memcpy(target, store->value_to_be_written, size);
                    return true;
                }
                node = node->next;
            }
        }
    }

    // Sample lock
    uint32_t lock = fetch_lock(region->lock);

    // Reads the value and store in read_set
    load_t load = new_load();
    if (!load) return false;
    load->read_address = source;
    node_t* new_node = create_node(load);
    add_node(transaction->read_set, new_node)

    memcpy(target, source, size);

    // Post validation
    if (get_lock_version(lock) > transaction->rv || get_lock_flag(lock) == LOCKED) {
        return false;
    }
    return true;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    transaction_t* transaction = (transaction_t*) tx;

    store_t store = new_store();
    if (!store) return false;
    store->address_to_be_written = target;
    store->size = size;
    memcpy(store->value_to_be_written, source, size);

    bloom_add(transaction->write_set_bloom_filter, &(store->address_to_be_written), sizeof(void*));
    node_t* new_node = create_node(store);
    add_node(transaction->write_set, new_node)
    return true;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
alloc_t tm_alloc(shared_t shared as(unused), tx_t tx as(unused), size_t size as(unused), void** target as(unused)) {
    // TODO: tm_alloc(shared_t, tx_t, size_t, void**)
    return abort_alloc;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t shared as(unused), tx_t tx as(unused), void* target as(unused)) {
    // TODO: tm_free(shared_t, tx_t, void*)
    return false;
}
