#include <stdlib.h>
#include <string.h>

#include "version_list_item.h"
#include "list.h"


version_list_item_t* create_version_list_item_empty(size_t align) {
    version_list_item_t* version_list_item = (version_list_item_t*) malloc(sizeof(version_list_item_t));
    version_list_item->tx_id = 1;
    version_list_item->value = calloc(1, align);
    version_list_item->read_list = NULL;
    return version_list_item;
}

version_list_item_t* create_version_list_item(transaction_t* transaction, void* value, size_t size) {
    version_list_item_t* version_list_item = (version_list_item_t*) malloc(sizeof(version_list_item_t));
    version_list_item->tx_id = transaction->tx_id;
    version_list_item->value = malloc(size);
    memcpy(version_list_item->value, value, size);
    version_list_item->read_list = NULL;
    return version_list_item;
}

void destroy_version_list_item(version_list_item_t* version_list_item) {
    if (version_list_item->value) {
        free(version_list_item->value);
    }

    if (version_list_item->read_list) {
        destroy_list(version_list_item->read_list, destroy_read_list_node);
    }

    free(version_list_item);
}

void add_reader(version_list_item_t* version_list_item, transaction_t* transaction) {
    if (!version_list_item->read_list) {
        version_list_item->read_list = create_list();
    }
    uint_t* tx_id = malloc(sizeof(uint_t));
    memcpy(tx_id, &(transaction->tx_id), sizeof(uint_t));
    node_t* node = create_node(tx_id);
    add_node(version_list_item->read_list, node);
}

void destroy_read_list_node(node_t* node) {
    if (node->content) {
        free(node->content);
    }

    free(node);
}
