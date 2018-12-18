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
    version_list->highest_tx_id = 0;

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

    if (version_list->highest_tx_id < version_list_item->tx_id) {
        version_list->highest_tx_id = version_list_item->tx_id;
    }
    node_t* node = create_node(version_list_item);
    add_node(version_list->list, node);
}

uint_t find_stl(version_list_t* version_list, uint_t tx_id) {
    uint_t stl = UINT_MAX;
    node_t* node = version_list->list->first;
    while (node) {
        version_list_item_t* version_list_item = (version_list_item_t*) node->content;
        if (version_list_item->tx_id > tx_id && version_list_item->tx_id < stl) {
            stl = version_list_item->tx_id;
        }
        node = node->next;
    }
    return stl;
}

bool check_terminated_transactions(region_t* region, version_list_t* version_list, uint_t tx_id) {
    uint_t stl = find_stl(version_list, tx_id);
    if (stl == UINT_MAX) return false;

    for (uint_t i = tx_id + 1; i < stl; i++) {
        if (!is_transaction_dead(region, i)) {
            return false;
        }
    }

    return true;
}


void init_garbage_collection(region_t* region, version_list_t* version_list) {
    list_t* nodes_to_delete = create_list();

    node_t* node = version_list->list->last;
    while (node) {
        version_list_item_t* version_list_item = (version_list_item_t*) node->content;
        bool checked = check_terminated_transactions(region, version_list, version_list_item->tx_id);
        if (version_list_item->tx_id < version_list->highest_tx_id && checked) {
            // Add to delete list
            node_t* node_to_delete = create_node(node);
            add_node(nodes_to_delete, node_to_delete);
        }
        node = node->previous;
    }

    // Delete nodes
    //printf("*** nDeletedNodes = %d\n", nodes_to_delete->size);
    node_t* node_to_delete = nodes_to_delete->first;
    while (node_to_delete) {
        node_t* n = (node_t*) node_to_delete->content;
        remove_node(version_list->list, n);
        destroy_version_list_node(n);
        node_to_delete = node_to_delete->next;
    }
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