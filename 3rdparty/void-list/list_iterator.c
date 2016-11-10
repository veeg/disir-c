
//
// iterator.c
//

#include <list.h>
#include "list_private.h"

//!
//! Allocate a new list_iterator_t with the given start
//! node. NULL on failure.
//!
static list_iterator_t *
list_iterator_new_from_node (list_node_t *node, list_direction_t direction)
{
    list_iterator_t *self;
    if (!(self = LIST_MALLOC(sizeof(list_iterator_t))))
        return NULL;

    self->next = node;
    self->direction = direction;

    return self;
}


//!
//! Allocate a new list_iterator_t. NULL on failure.
//! Accepts a direction, which may be LIST_HEAD or LIST_TAIL.
//!
list_iterator_t *
list_iterator_create (list_t *list, list_direction_t direction)
{
    list_node_t *node = direction == LIST_HEAD
        ? list->head
        : list->tail;
    return list_iterator_new_from_node(node, direction);
}

/*
 * Return the next list_node_t or NULL when no more
 * nodes remain in the list.
 */

void *
list_iterator_next (list_iterator_t *self)
{
    list_node_t *curr = self->next;

    if (curr)
    {
        self->next = self->direction == LIST_HEAD
            ? curr->next
            : curr->prev;
        return curr->val;
    }

    return NULL;
}

//!
//! Free the list iterator.
//!
void
list_iterator_destroy (list_iterator_t **self)
{
    LIST_FREE (*self);
    *self = NULL;
}

