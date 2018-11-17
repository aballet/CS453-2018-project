typedef struct node {
    struct node *prev;
    struct node *next;
    void* segment;
} node_t;

typedef struct list {
    node_t *first;
    int size;
} list_t;
