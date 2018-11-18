#include "transaction.h"

transaction_t* create_transaction(bool is_read_only) {
    transaction_t* transaction = (transaction_t*) malloc(sizeof(transaction_t));
    if (!transaction) return NULL;
    transaction->is_read_only = is_read_only;

    // Init read-set and write-set
    transaction->read_set = create_list();
    if (!transaction->read_set) {
        free(transaction);
        return NULL;
    }
    if (!is_read_only) {
        transaction->write_set = create_list();
        if (!transaction->write_set) {
            free(transaction->read_set);
            free(transaction);
            return NULL;
        }
        transaction->write_set_bloom_filter = (struct bloom*) malloc(sizeof(struct bloom));
        if (!transaction->write_set_bloom_filter) {
            free(transaction->write_set);
            free(transaction->read_set);
            free(transaction);
            return NULL;
        }
        if (bloom_init(transaction->write_set_bloom_filter, 100000, 0.01) != 0) {
            free(transaction->write_set_bloom_filter);
            free(transaction->write_set);
            free(transaction->read_set);
            free(transaction);
            return NULL;
        }
    }
    transaction->rv = 0;
    transaction->wv = 0;
    return transaction;
}

void destroy_transaction(transaction_t* transaction) {
    if (!transaction) return;
    if (transaction->read_set) {
        destroy_list(transaction->read_set, void (*destroy_node)(node_t));
        free(transaction->read_set);
    }
    if (transaction->write_set) {
        destroy_list(transaction->write_set, void (*destroy_node)(node_t));
        free(transaction->write_set);
    }
    if (transaction->write_set_bloom_filter) {
        bloom_free(transaction->write_set_bloom_filter);
        free(transaction->write_set_bloom_filter);
    }
}

load_t* new_load() {
    return malloc(sizeof(load_t));
}

store_t* new_store() {
    return malloc(sizeof(store_t));
}
