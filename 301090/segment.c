#include <stdlib.h>
#include <stdio.h>

#include "segment.h"
#include "lock.h"
#include "list.h"
#include "version_list.h"
#include "region.h"

segment_t* create_segment(size_t align) {
    segment_t* segment = (segment_t*) malloc(sizeof(segment_t));
    if (!segment) return NULL;

    segment->version_list = create_version_list(align);
    if (!segment->version_list) {
        free(segment);
        return NULL;
    }

    segment->lock = create_lock();
    if (!segment->lock) {
        destroy_version_list(segment->version_list);
        free(segment);
        return NULL;
    }

    return segment;
}

void destroy_segment(segment_t* segment) {
    if (segment->version_list) {
        destroy_version_list(segment->version_list);
    }

    if (segment->lock) {
        destroy_lock(segment->lock);
    }

    free(segment);
}

bool lock_segment(transaction_t* transaction, segment_t* segment) {
    return acquire_lock(transaction, segment->lock);
}

bool unlock_segment(transaction_t* transaction, segment_t* segment) {
    return release_lock(transaction, segment->lock);
}

void print_list(list_t* list) {
    node_t* node = list->first;
    while (node) {
        version_list_item_t* item = (version_list_item_t*) node->content;
        printf("%d ", item->tx_id);
        node = node->next;
    }
    printf("\n");
}

void garbage_collect(region_t* region, transaction_t* transaction, segment_t* segment) {
//    printf("segment locked? %d\n", segment->lock->tx_id == transaction->tx_id);
//    printf("live tx locked? %d\n", region->live_transactions_lock->tx_id == transaction->tx_id);
    list_t* version_list = segment->version_list->list;
    //printf("vl :\n");
    //print_list(version_list);
    node_t* node = version_list->first;

    list_t* nodes_to_delete = create_list();

    while (node) {
        version_list_item_t* version_list_item = (version_list_item_t*) node->content;
        if (!version_list_item->has_nts) {
            node = node->next;
            continue;
        }

        uint_t j = version_list_item->tx_id - 1;

        bool can_delete = true;
        while (j < version_list_item->nts) {
            if (is_transaction_live(region, j)) {
                can_delete = false;
                break;
            }
            j++;
        }

        if (can_delete) {
            node_t* node_to_delete = create_node(node);
            add_node(nodes_to_delete, node_to_delete);
        }

        node = node->next;
    }

    // Garbage collection
    node = nodes_to_delete->first;
    printf("Number of nodes in version_list : %d\n", version_list->size);
    printf("Number of nodes deleted : %d\n", nodes_to_delete->size);
    while (node) {
        node_t* node_to_delete = (node_t*) node->content;
        //printf("to delete : %d\n", ((version_list_item_t*) node_to_delete->content)->tx_id);
        remove_node(version_list, node_to_delete);
        //print_list(version_list);
        node = node->next;
    }
}
