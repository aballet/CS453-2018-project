#include <stdlib.h>
#include "double_linked_list.h"

node_t *init_node(void* segment) {
    node_t* node = (node_t*)malloc(sizeof(node_t));
    if(!node) {
        return NULL;
    }
    node->segment = segment;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void add_node(node_t* node) {

}

void remove_node(node_t* node) {

}
