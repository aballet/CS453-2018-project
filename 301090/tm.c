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
#include "tm.h"

#include <stdio.h>
#include <errno.h>

#include "list.h"
#include "lock.h"
#include "region.h"
#include "segment.h"
#include "transaction.h"
#include "version_list.h"
#include "version_list_item.h"
#include "hashset.h"
#include "hashset_itr.h"

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
// SIGNATURES


version_list_item_t* find_lts(transaction_t* transaction, segment_t* segment);

// -------------------------------------------------------------------------- //



shared_t tm_create(size_t size, size_t align) {
    setbuf(stdout, NULL);
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
    //printf("** START tm_begin\n");

    region_t* region = (region_t*) shared;
    transaction_t* transaction = create_transaction(region, is_ro);
    if (!transaction) return invalid_tx;

    return (tx_t) transaction;
}

bool check_versions(transaction_t* transaction, segment_t* segment) {
    version_list_t* version_list = segment->version_list;
    node_t* version_list_node = version_list->list->first;

    while (version_list_node) {
        version_list_item_t* version_list_item = (version_list_item_t*) version_list_node->content;
        if (version_list_item->has_been_read) {
            if (version_list_item->tx_id < transaction->tx_id && transaction->tx_id < version_list_item->readers_max_tx_id) {
                return false;
            }
        }
        version_list_node = version_list_node->next;
    }

    return true;
}

void insert_version_list_item(region_t* region, segment_t* segment, transaction_t* transaction, void* value, size_t size) {
    version_list_item_t* current_version_list_item = create_version_list_item(transaction, value, size);
    version_list_item_t* previous_version_list_item = find_lts(transaction, segment);

    if (previous_version_list_item) {
        if (previous_version_list_item->has_nts) {
            current_version_list_item->has_nts = false;
            //current_version_list_item->nts = previous_version_list_item->nts;
        }
        previous_version_list_item->has_nts = true;
        previous_version_list_item->nts = transaction->tx_id;
    }

    add_version_list_item(segment->version_list, current_version_list_item);
}

bool tm_end(shared_t shared, tx_t tx) {
    //printf("** START tm_end\n");

    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;

    if (transaction->is_read_only) {
        //printf("** END tm_end %d : read_only\n", transaction->tx_id);
        destroy_transaction(region, transaction);
        return true;
    }

    list_t* lock_set = create_list();

    list_t* write_set = transaction->write_set;
    node_t* node = write_set->first;

    while (node) {
        write_item_t* write_item = node->content;
        size_t start_index = get_segment_start_index(region, write_item->address);
        size_t end_index = get_segment_end_index(region, write_item->address, write_item->size);
        for (size_t i = start_index; i < end_index; i++) {
            segment_t* segment = get_segment(region, i);
            bool acquired = lock_segment(transaction, segment);
            if (acquired) {
                node_t* lock_node = create_node(segment);
                add_node(lock_set, lock_node);
            } else {
                node_t* lock_node = lock_set->first;
                while (lock_node) {
                    segment_t* locked_segment = (segment_t*) lock_node->content;
                    unlock_segment(transaction, locked_segment);
                    lock_node = lock_node->next;
                }
                // TODO: destroy lock_set
                //printf("** END tm_end %d : false\n", transaction->tx_id);
                destroy_transaction(region, transaction);
                return false;
            }
            if (!check_versions(transaction, segment)) {
                node_t* lock_node = lock_set->first;
                while (lock_node) {
                    segment_t* locked_segment = (segment_t*) lock_node->content;
                    unlock_segment(transaction, locked_segment);
                    lock_node = lock_node->next;
                }
                // TODO: destroy lock_set
                //printf("** END tm_end %d : false\n", transaction->tx_id);
                destroy_transaction(region, transaction);
                return false;
            }
        }
        node = node->next;
    }

    acquire_lock(transaction, region->live_transactions_lock);

    node = write_set->last;
    hashset_t hashset = hashset_create();
    while (node) {
        write_item_t* write_item = node->content;
        size_t start_index = get_segment_start_index(region, write_item->address);
        size_t end_index = get_segment_end_index(region, write_item->address, write_item->size);

        int j = 0;
        void* value = write_item->value;
        size_t align = region->align;
        for (size_t i = start_index; i < end_index; i++) {
            segment_t* segment = get_segment(region, i);
            hashset_add(hashset, segment);
            insert_version_list_item(region, segment, transaction, value + j * align, region->align);
            //print_version_list(segment->version_list);
            j++;
        }

        node = node->previous;
    }

    // Unlock
    node_t* lock_node = lock_set->first;
    while (lock_node) {
        segment_t* locked_segment = (segment_t*) lock_node->content;
        unlock_segment(transaction, locked_segment);
        lock_node = lock_node->next;
    }

    //printf("** END tm_end %d : true\n", transaction->tx_id);

    remove_live_transaction(region, transaction);
    acquire_lock(transaction, region->live_transactions_lock);

    hashset_itr_t hashset_itr = hashset_iterator(hashset);
    while (hashset_iterator_has_next(hashset_itr)) {
        segment_t* segment = (segment_t*) hashset_iterator_value(hashset_itr);
        if (segment->version_list->list->size > 20) {
            garbage_collect(region, transaction, segment);
        }
        hashset_iterator_next(hashset_itr);
    }

    destroy_transaction(region, transaction);

    return true;
}

version_list_item_t* find_lts(transaction_t* transaction, segment_t* segment) {
    version_list_t* version_list = segment->version_list;
    version_list_item_t* closest_version_list_item = NULL;

    node_t* node = version_list->list->first;
    while (node) {
        version_list_item_t* version_list_item = (version_list_item_t*) node->content;
        if (closest_version_list_item) {
            if (version_list_item->tx_id < transaction->tx_id && closest_version_list_item->tx_id < version_list_item->tx_id) {
                closest_version_list_item = version_list_item;
            }
        } else {
            if (version_list_item->tx_id < transaction->tx_id) {
                closest_version_list_item = version_list_item;
            }
        }
        node = node->next;
    }
    return closest_version_list_item;
}


// TODO : if fails, call tm_end
bool tm_read(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    //printf("** START tm_read\n");

    region_t* region = (region_t*) shared;
    transaction_t* transaction = (transaction_t*) tx;

    if (!transaction->is_read_only) {
        list_t* write_set = transaction->write_set;
        node_t* node = write_set->first;
        while (node) {
            write_item_t* write_item = (write_item_t*) node->content;
            if (write_item->address == source && write_item->size == size) {
                memcpy(target, write_item->value, size);
                return true;
            }
            node = node->next;
        }
    }

    list_t* lock_set = create_list();

    // Lock segments
    size_t start_index = get_segment_start_index(region, source);
    size_t end_index = get_segment_end_index(region, source, size);

    for (size_t i = start_index; i < end_index; i++) {
        segment_t* segment = get_segment(region, i);
        bool acquired = lock_segment(transaction, segment);
        if (acquired) {
            node_t* lock_node = create_node(segment);
            add_node(lock_set, lock_node);
        } else {
            node_t* lock_node = lock_set->first;
            while (lock_node) {
                segment_t* locked_segment = (segment_t*) lock_node->content;
                unlock_segment(transaction, locked_segment);
                lock_node = lock_node->next;
            }
            // TODO : Free lock_set
            destroy_transaction(region, transaction);
            return false;
        }
    }

    // Copy segments
    size_t j = 0;
    size_t align = region->align;
    for (size_t i = start_index; i < end_index; i++) {
        //printf("i = %zu\n", i);

        segment_t* segment = get_segment(region, i);
        version_list_item_t* lts = find_lts(transaction, segment);
        if (lts) {
            memcpy(target + j * align, lts->value, align);
            add_reader(lts, transaction);
        } else {
            memset(target + j * align, 0, align);
        }
        j++;
    }

    // Unlock segments
    node_t* lock_node = lock_set->first;
    while (lock_node) {
        segment_t* locked_segment = (segment_t*) lock_node->content;
        unlock_segment(transaction, locked_segment);
        lock_node = lock_node->next;
    }
    // TODO : Free lock_set
    //printf("** END tm_read\n");

    return true;
}

// TODO : if fails, call tm_end
bool tm_write(shared_t shared as(unused), tx_t tx as(unused), void const* source, size_t size, void* target) {
    transaction_t* transaction = (transaction_t*) tx;
    list_t* write_set = transaction->write_set;

    write_item_t* write_item = new_write_set_item(target, size, source);
    node_t* node = create_node(write_item);
    add_node(write_set, node);

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
