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

#include "region.h"
#include "global_counter.h"
#include "versioned_lock.h"
#include "transaction.h"

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
  region->counter = create_global_counter();
  if (!region->counter) {
      free(region->start);
      free(region);
      return invalid_shared;
  }
  // Init lock
  region->lock = create_versioned_lock();
  if (!region->lock) {
      destroy_global_counter(region->counter);
      free(region->start);
      free(region);
      return invalid_shared;
  }
  // Init shared_memory with 0
  memset(region->start, 0, size);

  // Finish initialization and return region
  atomic_init(&(region->tx_id), 1);
  region->size = size;
  region->align = align;
  return (shared_t) region;

}

void tm_destroy(shared_t shared) {
    region_t* region = (region_t*) shared;
    if (region) {
        if (region->start) {
            free(region->start);
            region->start = NULL;
        }
        // Destroy counter
        if (region->counter) {
            destroy_global_counter(region->counter);
        }
        // Destroy lock
        if (region->lock) {
            destroy_versioned_lock(region->lock);
        }
        free(region);
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

    if (!transaction->is_read_only) {
        // Lock write_set

        list_t* write_set = transaction->write_set;
        node_t* node = write_set->first;
        while (node) {
            bool acquired = acquire_versioned_lock(region->lock, transaction->tx_id);
            //printf("acquired? %d\n", acquired);
            if (!acquired) return false;
            node = node->next;
        }

        // Increment global version-clock
        transaction->wv = increment_and_fetch_global_counter(region->counter);

        if (transaction->rv + 1 != transaction->wv) {
            list_t* read_set = transaction->read_set;
            node = read_set->first;
            while (node) {
                if (get_versioned_lock_tx_id(region->lock) != transaction->tx_id) {
                    // TODO : adapt to multiple locations
                    release_versioned_lock(region->lock, transaction->wv);
                    return false;
                }
                node = node->next;
            }
        }

        // Commit and release the locks
        node = write_set->last;
        while (node) {
            store_t* store = (store_t*) node->content;
            memcpy(store->address_to_be_written, store->value_to_be_written, store->size);
            node = node->previous;
        }
        // TODO : adapt to multiple locations
        release_versioned_lock(region->lock, transaction->wv);
    }
    //printf("SIZE OF READ SET = %d\n", transaction->read_set->size);
    //printf("SIZE OF WRITE SET = %d\n", transaction->write_set->size);
    return true;
}

bool tm_read_helper(region_t* region, transaction_t* transaction, void const* source, size_t size, void* target, void* value_to_be_written) {
    // Reads the value and store in read_set
    load_t* load = new_load();
    if (!load) return false;
    load->read_address = source;
    node_t* new_node = create_node(load);
    add_node(transaction->read_set, new_node);

    if (value_to_be_written) {
        memcpy(target, value_to_be_written, size);
    } else {
        memcpy(target, source, size);
    }

    // Post validation
    //printf("POST VALIDATION\n");
    //printf("lock_version? %d\n", get_lock_version(region->lock));
    //printf("rv? %d\n", transaction->rv);
    //printf("lock_flag? %d\n", get_lock_flag(region->lock));
    if (get_versioned_lock_version(region->lock) > transaction->rv || get_versioned_lock_tx_id(region->lock) != transaction->tx_id) {
        return false;
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
                    return tm_read_helper(region, transaction, source, size, target, store->value_to_be_written);
                }
                node = node->next;
            }
        //}
    }
    return tm_read_helper(region, transaction, source, size, target, NULL);

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
