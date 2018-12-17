#include <stdlib.h>
#include <stdio.h>

#include "version_list.h"
#include "list.h"
#include "version_list_item.h"

version_list_t* create_version_list(size_t align) {
    version_list_t* version_list = (version_list_t*) malloc(sizeof(version_list_t));
    version_list->list = create_list();

    // Init first node
    version_list_item_t* version_list_item = create_version_list_item_empty(align);
    node_t* node = create_node(version_list_item);
    add_node(version_list->list, node);

    return version_list;
}

void destroy_version_list(version_list_t* version_list) {
    if (version_list->list) {
        destroy_list(version_list->list, destroy_version_list_node);
    }
    free(version_list);
}

void destroy_version_list_node(node_t* node) {
    version_list_item_t* version_list_item = (version_list_item_t*) node->content;
    destroy_version_list_item(version_list_item);
}

void add_version_list_item(version_list_t* version_list, version_list_item_t* version_list_item) {
    if (!version_list->list) {
        version_list->list = create_list();
    }

    node_t* node = create_node(version_list_item);
    add_node(version_list->list, node);
}

void print_version_list(version_list_t* version_list) {
    printf("*** VERSION_LIST ***\n");
    node_t* node = version_list->list->first;
    while (node) {
        version_list_item_t* version_list_item = node->content;
        printf("%d[", version_list_item->tx_id);
        list_t* read_list = version_list_item->read_list;
        if (read_list) {
            node_t* node2 = read_list->first;
            while (node2) {
                uint_t* id = (uint_t*) node2->content;
                printf("%d,", *id);
                node2 = node2->next;
            }
        }
        printf("]\n");
        node = node->next;
    }
}