#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "transaction.h"
#include "list.h"
#include "region.h"
#include "lock.h"

transaction_t* create_transaction(region_t* region, bool is_read_only) {
    transaction_t* transaction = (transaction_t*) malloc(sizeof(transaction_t));
    if (!transaction) return NULL;
    transaction->is_read_only = is_read_only;

    // Init transaction id
    transaction->tx_id = increment_and_fetch_tx_id(region);

    if (!is_read_only) {
        // Init write_set
        transaction->write_set = create_list();
        if (!transaction->write_set) {
            free(transaction);
            return NULL;
        }
    }

    add_live_transaction(region, transaction);
    //printf("added tx %d, is live? %d\n", transaction->tx_id, is_transaction_live(region, transaction->tx_id));

    return transaction;
}

void destroy_transaction(region_t* region, transaction_t* transaction) {
    if (!transaction) return;

    if (!transaction->is_read_only && transaction->write_set) {
        destroy_list(transaction->write_set, destroy_write_set_node);
    }

    remove_live_transaction(region, transaction);
    //printf("removed tx %d, is live? %d\n", transaction->tx_id, is_transaction_live(region, transaction->tx_id));

    free(transaction);
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

void destroy_write_set_node(node_t* node) {
    write_item_t* write_item = (write_item_t*) node->content;
    destroy_write_set_item(write_item);
    node->content = NULL;
    free(node);
}
