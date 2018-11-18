#include <stdlib.h>
#include "linked_list.h"

list_t* create_list() {
    list_t* list = (list_t*) malloc(sizeof(list_t));
    if (!list) return NULL;
    list->first = NULL;
    list->last = NULL;
    list->size = 0;
    return list;
}

node_t* create_node(void* content) {
    node_t* node = (node_t*) malloc(sizeof(node_t));
    if (!node) return NULL;
    node->previous = NULL;
    node->next = NULL;
    node->content = segment;
    return node;
}

void add_node(list_t* list, node_t* node) {
    if (list->first == NULL) {
        list->first = node;
        list->last = node;
    } else {
        node_t* first = list->first;
        first->previous = node;
        list->first = node;
        node->next = first;
    }
    list->size += 1;
}

void destroy_list(list_t* list, void (*destroy_node)(node_t)) {
    node_t* node = list->first;
    while (node) {
        node_t* next_node = node->next;
        destroy_node(node);
        node = next_node;
    }
    list->first == NULL;
    list->last == NULL;
}
