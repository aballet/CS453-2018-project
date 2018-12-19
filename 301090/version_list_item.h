#ifndef VERSION_LIST_ITEM_H
#define VERSION_LIST_ITEM_H

#include "own_types.h"


version_list_item_t* create_version_list_item(transaction_t* transaction, void* value, size_t size);
void destroy_version_list_item(version_list_item_t* version_list_item);
void add_reader(version_list_item_t* version_list_item, transaction_t* transaction);


#endif /* VERSION_LIST_ITEM_H */
