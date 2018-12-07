#ifndef VERSION_LIST_ITEM_H
#define VERSION_LIST_ITEM_H

#include "version_list_item.h"
#include "own_types.h"


typedef struct version_list_item {
    uint_t tx_id;
    void* value;
    list_t* read_list;
} version_list_item_t;


#endif VERSION_LIST_ITEM_H
