#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "own_types.h"

transaction_t* create_transaction(region_t* region, bool is_read_only);
void destroy_transaction(region_t* region, transaction_t* transaction);
write_item_t* new_write_set_item(void* address, size_t size, void const* value);
void destroy_write_set_item(write_item_t* write_item);
void destroy_write_set_node(node_t* node);


#endif /* TRANSACTION_H */
