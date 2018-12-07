// Compile-time configuration
// #define USE_MM_PAUSE
// #define USE_TICKET_LOCK

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if (defined(__i386__) || defined(__x86_64__)) && defined(USE_MM_PAUSE)
    #include <xmmintrin.h>
#else
    #include <sched.h>
#endif

// Internal headers
#include <tm.h>

#include <stdio.h>
#include <errno.h>

#include "region.h"
#include "global_counter.h"
#include "versioned_lock.h"
#include "transaction.h"
#include "list.h"

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

/** Pause for a very short amount of time.
**/
static inline void pause() {
#if (defined(__i386__) || defined(__x86_64__)) && defined(USE_MM_PAUSE)
    _mm_pause();
#else
    sched_yield();
#endif
}

// -------------------------------------------------------------------------- //

shared_t tm_create(size_t size, size_t align) {
    region_t* region = create_region(size, align);
    if (!region) return invalid_shared;
    return (shared_t) region;
}

void tm_destroy(shared_t shared) {
    region_t* region = (region_t*) shared;
    if (region) {
        destroy_region(region);
    }
}

void* tm_start(shared_t shared) {
    region_t* region = (region_t*) shared;
    return region->start;
}

size_t tm_size(shared_t shared) {
    region_t* region = (region_t*) shared;
    return region->size;
}

size_t tm_align(shared_t shared) {
    region_t* region = (region_t*) shared;
    return region->align;
}

tx_t tm_begin(shared_t shared, bool is_ro) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = create_transaction(region, is_ro);
    if (!transaction) return invalid_tx;

    // Sample global version-clock
    transaction->rv = fetch_global_counter(region->counter);
    return (tx_t) transaction;
}

bool tm_end(shared_t shared, tx_t tx) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;

    //printf("Size of READ_SET = %d\n", transaction->read_set->size);
    //printf("Size of WRITE_SET = %d\n", transaction->write_set->size);
    if (!transaction->is_read_only) {
        // Lock write_set
        list_t* acquired_locks = create_list();
        list_t* write_set = transaction->write_set;
        node_t* write_node = write_set->first;
        while (write_node) {
            store_t* store = (store_t*) write_node->content;
            int start_index = get_locks_start_index(region, store->address_to_be_written);
            int end_index = get_locks_end_index(region, store->address_to_be_written, store->size);

            for (int i = start_index; i <= end_index; i++) {
                bool acquired = acquire_versioned_lock((region->locks)[i], transaction->tx_id);
                if (acquired) {
                    //printf("TRY TO ACQUIRE\n");
                    //printf("LOCK ID = %d\n", i);
                    //print_versioned_lock((region->locks)[i]);
                    node_t* lock_node = create_node((region->locks)[i]);
                    add_node(acquired_locks, lock_node);
                } else {
                    // Release every acquired lock and abort
                    node_t* node_to_release = acquired_locks->first;
                    while (node_to_release) {
                        versioned_lock_t* lock_to_release = (versioned_lock_t*) node_to_release->content;
                        release_versioned_lock_untouched(lock_to_release, transaction->tx_id);
                        node_to_release = node_to_release->next;
                    }
                    return false;
                }
            }
            write_node = write_node->next;
        }

        // Increment global version-clock
        transaction->wv = increment_and_fetch_global_counter(region->counter);

        // Validate read_set
        if (transaction->rv + 1 != transaction->wv) {
            list_t* read_set = transaction->read_set;
            node_t* read_node = read_set->first;
            while (read_node) {
                load_t* load = (load_t*) read_node->content;
                int start_index = get_locks_start_index(region, load->read_address);
                int end_index = get_locks_end_index(region, load->read_address, load->size);

                for (int i = start_index; i <= end_index; i++) {
                    // If lock.version > rv OR locked by another tx ==> abort
                    versioned_lock_t* lock_to_validate = (region->locks)[i];
                    if (get_versioned_lock_version(lock_to_validate) > transaction->rv || (get_versioned_lock_tx_id(lock_to_validate) != transaction->tx_id && get_versioned_lock_tx_id(lock_to_validate) != 0)) {
                        // Release every acquired lock and abort
                        node_t* node_to_release = acquired_locks->first;
                        while (node_to_release) {
                            versioned_lock_t* lock_to_release = (versioned_lock_t*) node_to_release->content;
                            release_versioned_lock_untouched(lock_to_release, transaction->tx_id);
                            node_to_release = node_to_release->next;
                        }
                        return false;
                    }
                }
                read_node = read_node->next;
            }
        }

        // Commit and release the locks
        write_node = write_set->last;
        while (write_node) {
            store_t* store = (store_t*) write_node->content;
            // Write value
            memcpy(store->address_to_be_written, store->value_to_be_written, store->size);
            write_node = write_node->previous;
        }
        // Release locks
        node_t* node_to_release = acquired_locks->first;
        while (node_to_release) {
            versioned_lock_t* lock_to_release = (versioned_lock_t*) node_to_release->content;
            release_versioned_lock(lock_to_release, transaction->tx_id, transaction->wv);
            node_to_release = node_to_release->next;
        }
    }
    return true;
}

bool tm_read_post_validation(region_t* region, transaction_t* transaction, const void* address, size_t size) {
    // Post validation
    int start_index = get_locks_start_index(region, address);
    int end_index = get_locks_end_index(region, address, size);

    //printf("start_index: %d\n", start_index);
    //printf("end_index: %d\n", end_index);

    for (int i = start_index; i <= end_index; i++) {
        versioned_lock_t* lock_to_validate = (region->locks)[i];
        //printf("POST VALIDATION\n");
        //printf("lock_version? %d\n", get_versioned_lock_version(lock_to_validate));
        //printf("rv? %d\n", transaction->rv);
        //printf("lock tx_id? %d\n", get_versioned_lock_tx_id(lock_to_validate));

        //print_versioned_lock(lock_to_validate);
        if (get_versioned_lock_version(lock_to_validate) > transaction->rv || get_versioned_lock_tx_id(lock_to_validate) != 0) {
            return false;
        }
    }
    return true;
}


// TODO : if fails, call tm_end
bool tm_read(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;
    // Check if load read_address already appears in the write_set
    if (!transaction->is_read_only) {
        // No write_set to check if transaction is read-only
        //int found = bloom_check(transaction->write_set_bloom_filter, &source, sizeof(void*));
        //if (found) {
            list_t* write_set = transaction->write_set;
            node_t* node = write_set->first;
            while (node) {
                store_t* store = (store_t*) node->content;
                if (store->address_to_be_written == source) {
                    // Return value found
                    // Reads the value and store in read_set
                    load_t* load = new_load(size);
                    if (!load) return false;
                    load->read_address = source;
                    node_t* new_node = create_node(load);
                    if (!new_node) return false;
                    add_node(transaction->read_set, new_node);
                    memcpy(target, store->value_to_be_written, size);
                    return tm_read_post_validation(region, transaction, source, size);
                }
                node = node->next;
            }
        //}
        load_t* load = new_load(size);
        if (!load) return false;
        load->read_address = source;
        node_t* new_node = create_node(load);
        if (!new_node) return false;
        add_node(transaction->read_set, new_node);
        memcpy(target, source, size);
        return tm_read_post_validation(region, transaction, source, size);
    }
    memcpy(target, source, size);
    return tm_read_post_validation(region, transaction, source, size);
}

// TODO : if fails, call tm_end
bool tm_write(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    transaction_t* transaction = (transaction_t*) tx;

    store_t* store = new_store(size);
    if (!store) return false;
    store->address_to_be_written = target;
    memcpy(store->value_to_be_written, source, size);

    //bloom_add(transaction->write_set_bloom_filter, &(store->address_to_be_written), sizeof(void*));

    node_t* new_node = create_node(store);
    if (!new_node) return false;
    add_node(transaction->write_set, new_node);
    return true;
}

// TODO : if fails, call tm_end
alloc_t tm_alloc(shared_t shared, tx_t tx as(unused), size_t size, void** target) {
  return abort_alloc;
}

// TODO : if fails, call tm_end
bool tm_free(shared_t shared, tx_t tx as(unused), void* segment) {
  return false;
}
