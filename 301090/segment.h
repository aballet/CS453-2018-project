#ifndef SEGMENT_H
#define SEGMENT_H

#include <stdlib.h>
#include <bool.h>


typedef struct segment {
    version_list_t* version_list;
    lock_t* lock;
} segment_t;

segment_t* create_segment();
void destroy_segment(segment_t* segment);
bool lock_segment(transaction_t* transaction, segment_t* segment);
bool unlock_segment(transaction_t* transaction, segment_t* segment);


#endif /* SEGMENT_H */
