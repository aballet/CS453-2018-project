#include "version_list.h"

version_list_t* create_version_list() {
    version_list_t* version_list = (version_list_t*) malloc(sizeof(version_list_t);
    version_list->list = create_list();

    // Init first node
    
}

void destroy_version_list(version_list_t* version_list) {
    if (version_list->list) {
        destroy_list(version_list->list);
    }
    free(version_list);
}
