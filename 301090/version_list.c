#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "version_list.h"
#include "list.h"
#include "version_list_item.h"
#include "region.h"

version_list_t* create_version_list(size_t align) {
    version_list_t* version_list = (version_list_t*) malloc(sizeof(version_list_t));
    version_list->list = create_list();

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
        printf("%d[%d]\n", version_list_item->tx_id, version_list_item->readers_max_tx_id);
        node = node->next;
    }
}