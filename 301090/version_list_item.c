#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "version_list_item.h"
#include "list.h"


version_list_item_t* create_version_list_item(transaction_t* transaction, void* value, size_t size) {
    version_list_item_t* version_list_item = (version_list_item_t*) malloc(sizeof(version_list_item_t));
    version_list_item->tx_id = transaction->tx_id;
    version_list_item->value = malloc(size);
    memcpy(version_list_item->value, value, size);
    version_list_item->has_been_read = false;
    version_list_item->readers_max_tx_id = 0;
    version_list_item->has_nts = false;
    version_list_item->nts = 0;
    return version_list_item;
}

void destroy_version_list_item(version_list_item_t* version_list_item) {
    if (version_list_item->value) {
        free(version_list_item->value);
    }

    free(version_list_item);
}

void add_reader(version_list_item_t* version_list_item, transaction_t* transaction) {
    if (version_list_item->has_been_read) {
        if (transaction->tx_id > version_list_item->readers_max_tx_id) {
            version_list_item->readers_max_tx_id = transaction->tx_id;
        }
    } else {
        version_list_item->has_been_read = true;
        version_list_item->readers_max_tx_id = transaction->tx_id;
    }
}
