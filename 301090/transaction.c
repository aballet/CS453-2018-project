#include "transaction.h"

void destroy_read_node(node_t* node) {
    load_t* content = (load_t*) node->content;
    free(content);
    node->content = NULL;
    free(node);
}

void destroy_write_node(node_t* node) {
    store_t* content = (store_t*) node->content;
    free(content->value_to_be_written);
    content->value_to_be_written = NULL;
    free(content);
    node->content = NULL;
    free(node);
}

transaction_t* create_transaction(region_t* region, bool is_read_only) {
    transaction_t* transaction = (transaction_t*) malloc(sizeof(transaction_t));
    if (!transaction) return NULL;
    transaction->is_read_only = is_read_only;

    // Init transaction id
    transaction->tx_id = increment_and_fetch_tx_id(region);

    if (!is_read_only) {
        // Init read-set and write-set
        transaction->read_set = create_list();
        if (!transaction->read_set) {
            free(transaction);
            return NULL;
        }

        transaction->write_set = create_list();
        if (!transaction->write_set) {
            destroy_list(transaction->read_set, destroy_read_node);
            free(transaction);
            return NULL;
        }
        // transaction->write_set_bloom_filter = (struct bloom*) malloc(sizeof(struct bloom));
        // if (!transaction->write_set_bloom_filter) {
        //     destroy_list(transaction->write_set);
        //     destroy_list(transaction->read_set);
        //     free(transaction);
        //     return NULL;
        // }
        // if (bloom_init(transaction->write_set_bloom_filter, 10000, 0.01) != 0) {
        //     //free(transaction->write_set_bloom_filter);
        //     destroy_list(transaction->write_set);
        //     destroy_list(transaction->read_set);
        //     free(transaction);
        //     return NULL;
        // }
    }
    transaction->rv = 0;
    transaction->wv = 0;
    return transaction;
}

void destroy_transaction(transaction_t* transaction) {
    if (!transaction) return;
    if (transaction->read_set) {
        destroy_list(transaction->read_set, destroy_read_node);
        free(transaction->read_set);
    }
    if (transaction->write_set) {
        destroy_list(transaction->read_set, destroy_write_node);
        free(transaction->write_set);
    }
    // if (transaction->write_set_bloom_filter) {
    //     bloom_free(transaction->write_set_bloom_filter);
    //     free(transaction->write_set_bloom_filter);
    // }
}

load_t* new_load(size_t size) {
    load_t* load = (load_t*) malloc(sizeof(load_t));
    load->size = size;
    return load;
}

store_t* new_store(size_t size) {
    store_t* store = (store_t*) malloc(sizeof(store_t));
    store->value_to_be_written = malloc(size);
    store->size = size;
    return store;
}
