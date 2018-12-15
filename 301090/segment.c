#include <stdlib.h>

#include "segment.h"
#include "lock.h"
#include "version_list.h"

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
