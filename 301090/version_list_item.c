#include "version_list_item.h"


version_list_item_t* create_version_list_item_empty() {
    version_list_item_t* version_list_item = (version_list_item_t*) malloc(sizeof(version_list_item_t));
    version_list_item->tx_id = 0;
    version_list_item->value = NULL;
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
        free(value);
    }

    if (version_list_item->read_list) {
        destroy_list(version_list_item->read_list, destroy_read_list_node);
    }
}

void add_reader(version_list_item_t* version_list_item, transaction_t* transaction) {
    if (!version_list_item->read_list) {
        version_list_item->read_list = create_list();
    }
    node_t* node = create_node(&(transaction->tx_id));
    add_node(version_list_item->read_list, node);
}

void destroy_read_list_node(node_t* node) {
    node->content = NULL;
    free(node);
}
