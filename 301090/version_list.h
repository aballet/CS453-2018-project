#ifndef VERSION_LIST_H
#define VERSION_LIST_H

#include "own_types.h"

version_list_t* create_version_list(size_t align);
void destroy_version_list(version_list_t* version_list);
void destroy_version_list_node(node_t* node);
void add_version_list_item(version_list_t* version_list, version_list_item_t* version_list_item);
void print_version_list(version_list_t* version_list);
void init_garbage_collection(region_t* region, version_list_t* version_list);


#endif /* VERSION_LIST_H */
