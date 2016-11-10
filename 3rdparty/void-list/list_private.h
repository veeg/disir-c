

//! LIST_NODE ADT
typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
    void *val;
} list_node_t;

//! LIST ADT
struct list {
    list_node_t *head;
    list_node_t *tail;
    unsigned int len;
    void (*free)(void *val);
    int (*match)(void *a, void *b);
};


//! LIST_ITERATOR ADT
struct list_iterator {
    list_node_t *next;
    list_direction_t direction;
};



list_node_t *
list_node_new (void *value);

