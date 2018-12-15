#ifndef OWN_TYPES_H
#define OWN_TYPES_H

#include <stdatomic.h>
#include <stdbool.h>



/* UINT */

typedef unsigned int uint_t;



/* LIST */

typedef struct node {
    struct node *previous;
    struct node *next;
    void* content;
} node_t;

typedef struct list {
    node_t *first;
    node_t *last;
    int size;
} list_t;



/* LOCK */

typedef struct lock {
    atomic_uint tx_id;
} lock_t;



/* TRANSACTION */

typedef struct transaction {
    uint_t tx_id;
    bool is_read_only;
    list_t* read_set;
    list_t* write_set;
} transaction_t;

typedef struct read_item {
    void* address;
    void* value;
    size_t size;
} read_item_t;

typedef struct write_item {
    void* address;
    void* value;
    size_t size;
} write_item_t;



/* VERSION_LIST*/

typedef struct version_list {
    list_t* list;
} version_list_t;



/* VERSION_LIST_ITEM */

typedef struct version_list_item {
    uint_t tx_id;
    void* value;
    list_t* read_list;
} version_list_item_t;



/* SEGMENT */

typedef struct segment {
    version_list_t* version_list;
    lock_t* lock;
} segment_t;



/* REGION */

typedef struct region {
    void* start;
    size_t size;
    size_t align;
    atomic_uint tx_id;
    size_t n_segments;
    segment_t** segments;
} region_t;


#endif /* OWN_TYPES_H */
