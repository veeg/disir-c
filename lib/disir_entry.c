#include <stdlib.h>

#include <disir/disir.h>

//! PUBLIC API
enum disir_status
disir_entry_finished (struct disir_entry **entry)
{

    if (entry == NULL || *entry == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Remove it from queue.
    if ((*entry)->next)
    {
        // We are not the tail
        (*entry)->next->prev = (*entry)->prev;
    }
    else if ((*entry)->prev != NULL)
    {
        // We are the last element of the queue
        // We have to retrieve the head, and set the prev to our prev
        struct disir_entry *current = (*entry)->prev;
        while (current != NULL)
        {
            // Current is head
            if (current->prev == (*entry))
            {
                break;
            }
            current = current->prev;
        }
        current->prev = (*entry)->prev;
    }
    if ((*entry)->prev)
    {
        // If we are freeing head - we must not set the next to NULL
        if ((*entry)->prev->next != NULL)
        {
            (*entry)->prev->next = (*entry)->next;
        }
    }

    if ((*entry)->de_entry_name)
    {
        free ((*entry)->de_entry_name);
    }

    free (*entry);
    *entry = NULL;
    return DISIR_STATUS_OK;
}

