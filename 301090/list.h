#ifndef LIST_H
#define LIST_H

typedef struct node {
    struct node *previous;
    struct node *next;
    void* content;
} node_t;

typedef struct list {
    node_t *first;
    node_t *last;
    int size;
} list_t;

list_t* create_list();
node_t* create_node(void* content);
void add_node(list_t* list, node_t* node);
void destroy_list(list_t* list, void (*destroy_node)(node_t*));

#endif /* LIST_H */
