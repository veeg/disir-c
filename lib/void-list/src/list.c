
//
// list.c
//

#include <list.h>
#include "list_private.h"

list_node_t *
list_node_new (void *val)
{
    list_node_t *self;

    self = LIST_MALLOC (sizeof (list_node_t));
    if (self == NULL)
        return NULL;

    self->prev = NULL;
    self->next = NULL;
    self->val = val;

    return self;
}

//!
//! Allocate a new list_t. NULL on failure.
//!
list_t *
list_create (void)
{
    list_t *self;

    self = LIST_MALLOC (sizeof (list_t));
    if (self == NULL)
        return NULL;

    self->head = NULL;
    self->tail = NULL;
    self->free = NULL;
    self->match = NULL;
    self->len = 0;

    return self;
}

//!
//! Free the list.
//!
void
list_destroy (list_t **self)
{
    unsigned int len = (*self)->len;
    list_node_t *next;
    list_node_t *curr = (*self)->head;

    while (len--)
    {
        next = curr->next;
        if ((*self)->free)
        {
            (*self)->free(curr->val);
        }
        LIST_FREE (curr);
        curr = next;
    }

    LIST_FREE (*self);
    *self = NULL;
}

//!
//! Append the given node to the list
//! and return the node, NULL on failure.
//!
int
list_rpush (list_t *self, void *value)
{
    list_node_t *node;

    if (!value)
        return (-1);

    node = list_node_new (value);
    if (!node)
        return (-1);

    if (self->len) {
        node->prev = self->tail;
        node->next = NULL;
        self->tail->next = node;
        self->tail = node;
    } else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    return (0);
}

//!
//! Return / detach the last node in the list, or NULL.
//!
void *
list_rpop (list_t *self)
{
    if (!self->len) return NULL;

    list_node_t *node = self->tail;

    if (--self->len) {
        (self->tail = node->prev)->next = NULL;
    } else {
        self->tail = self->head = NULL;
    }

    node->next = node->prev = NULL;
    return node->val;
}

//!
//! Return / detach the first node in the list, or NULL.
//!
void *
list_lpop (list_t *self)
{
    if (!self->len) return NULL;

    list_node_t *node = self->head;

    if (--self->len) {
        (self->head = node->next)->prev = NULL;
    } else {
        self->head = self->tail = NULL;
    }

    node->next = node->prev = NULL;
    return node->val;
}

//!
//! Prepend the given node to the list
//! and return the node, NULL on failure.
//!
int
list_lpush(list_t *self, void *value)
{
    list_node_t *node;

    if (!value)
        return (-1);

    node = list_node_new (value);
    if (!node)
        return (-1);

    if (self->len) {
        node->next = self->head;
        node->prev = NULL;
        self->head->prev = node;
        self->head = node;
    } else {
        self->head = self->tail = node;
        node->prev = node->next = NULL;
    }

    ++self->len;
    return 0;
}

//!
//! Return the node associated to val or NULL.
//!
void *
list_value_exist (list_t *self, void *val)
{
    list_iterator_t *it = list_iterator_create (self, LIST_HEAD);
    list_node_t *node;
    void *ret;

    ret = NULL;

    while ((node = list_iterator_next(it))) {
        if (self->match) {
            if (self->match(val, node->val)) {
                ret = val;
                break;
            }
        } else {
            if (val == node->val) {
                ret = val;
                break;
            }
        }
    }

    list_iterator_destroy (&it);
    return ret;
}

//!
//! Return the value at the given index or NULL.
//!
void *
list_at (list_t *self, int index)
{
    list_direction_t direction = LIST_HEAD;

    if (index < 0)
    {
        direction = LIST_TAIL;
        index = ~index;
    }

    if ((unsigned)index < self->len)
    {
        list_iterator_t *it = list_iterator_create (self, direction);
        list_node_t *node = list_iterator_next (it);
        while (index--) node = list_iterator_next (it);
        list_iterator_destroy (&it);
        return node->val;
    }

    return NULL;
}

//!
//! Remove the node with the given value from the list, freeing it and it's value.
//!
int
list_remove (list_t *self, void *value)
{
    list_node_t *node;

    node = self->head;
    while (node != NULL)
    {
        if (node->val == value)
            break;
        node = node->next;
    }
    if (!node)
        return (-1);

    node->prev
        ? (node->prev->next = node->next)
        : (self->head = node->next);

    node->next
        ? (node->next->prev = node->prev)
        : (self->tail = node->prev);

    if (self->free)
    {
        self->free (node->val);
    }

    LIST_FREE (node);
    --self->len;

    return (0);
}

