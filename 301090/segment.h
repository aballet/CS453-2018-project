#ifndef SEGMENT_H
#define SEGMENT_H

#include "own_types.h"

segment_t* create_segment(size_t align);
void destroy_segment(segment_t* segment);
bool lock_segment(transaction_t* transaction, segment_t* segment);
bool unlock_segment(transaction_t* transaction, segment_t* segment);


#endif /* SEGMENT_H */
