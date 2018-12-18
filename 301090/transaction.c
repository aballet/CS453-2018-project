#include <stdlib.h>
#include <string.h>

#include "transaction.h"
#include "list.h"
#include "region.h"

transaction_t* create_transaction(region_t* region, bool is_read_only) {
    transaction_t* transaction = (transaction_t*) malloc(sizeof(transaction_t));
    if (!transaction) return NULL;
    transaction->is_read_only = is_read_only;

    // Init transaction id
    transaction->tx_id = increment_and_fetch_tx_id(region);

    //if (!is_read_only) {
    // Init read_set
    transaction->read_set = create_list();
    if (!transaction->read_set) {
        free(transaction);
        return NULL;
    }

    // Init write_set
    transaction->write_set = create_list();
    if (!transaction->write_set) {
        destroy_list(transaction->read_set, destroy_read_set_node);
        free(transaction);
        return NULL;
    }
    //}
    return transaction;
}

void destroy_transaction(region_t* region, transaction_t* transaction) {
    if (!transaction) return;

    if (transaction->read_set) {
        destroy_list(transaction->read_set, destroy_read_set_node);
    }

    if (transaction->write_set) {
        destroy_list(transaction->write_set, destroy_write_set_node);
    }

    add_dead_transaction(region, transaction);

    free(transaction);
}

read_item_t* new_read_set_item(void* address, size_t size, void* value) {
    read_item_t* read_item = (read_item_t*) malloc(sizeof(read_item_t));
    read_item->address = address;
    read_item->size = size;
    read_item->value = malloc(size);
    if (!read_item->value) {
        free(read_item);
        return NULL;
    }
    memcpy(read_item->value, value, size);
    return read_item;
}

void destroy_read_set_item(read_item_t* read_item) {
    free(read_item->value);
    free(read_item);
}

write_item_t* new_write_set_item(void* address, size_t size, void const* value) {
    write_item_t* write_item = (write_item_t*) malloc(sizeof(write_item_t));
    if (!write_item) return NULL;
    write_item->address = address;
    write_item->size = size;
    write_item->value = malloc(size);
    if (!write_item->value) {
        free(write_item);
        return NULL;
    }
    memcpy(write_item->value, value, size);

    return write_item;
}

void destroy_write_set_item(write_item_t* write_item) {
    free(write_item->value);
    free(write_item);
}

void destroy_read_set_node(node_t* node) {
    read_item_t* read_item = (read_item_t*) node->content;
    destroy_read_set_item(read_item);
    node->content = NULL;
    free(node);
}

void destroy_write_set_node(node_t* node) {
    write_item_t* write_item = (write_item_t*) node->content;
    destroy_write_set_item(write_item);
    node->content = NULL;
    free(node);
}
