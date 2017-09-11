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
        (*entry)->next->prev = (*entry)->prev;
    }
    if ((*entry)->prev)
    {
        (*entry)->prev->next = (*entry)->next;
    }

    if ((*entry)->de_entry_name)
    {
        free ((*entry)->de_entry_name);
    }

    free (*entry);
    *entry = NULL;
    return DISIR_STATUS_OK;
}

