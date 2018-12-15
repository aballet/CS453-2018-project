#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "own_types.h"

transaction_t* create_transaction(region_t* region, bool is_read_only);
void destroy_transaction(transaction_t* transaction);
read_item_t* new_read_set_item(void* address, size_t size, void* value);
void destroy_read_set_item(read_item_t* read_item);
write_item_t* new_write_set_item(void* address, size_t size, void const* value);
void destroy_write_set_item(write_item_t* write_item);
void destroy_read_set_node(node_t* node);
void destroy_write_set_node(node_t* node);


#endif /* TRANSACTION_H */
