#ifndef LIST_H
#define LIST_H


#include "own_types.h"

list_t* create_list();
node_t* create_node(void* content);
void add_node(list_t* list, node_t* node);
void destroy_list(list_t* list, void (*destroy_node)(node_t*));


#endif /* LIST_H */
