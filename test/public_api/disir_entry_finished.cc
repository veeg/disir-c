
// PUBLIC API
#include <disir/disir.h>
#include <disir/plugin.h>

#include "test_helper.h"


//
// This class tests the public API functions:
//  disir_plugin_registered
//  disir_plugin_finished
//
class EntryFinished : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp();

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        struct disir_entry *entry = queue;
        struct disir_entry *current = queue;
        while (entry)
        {
            current = entry;
            entry = entry->next;
            disir_entry_finished (&current);
        }
        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status = DISIR_STATUS_OK;
    struct disir_entry *queue = NULL;
};

TEST_F (EntryFinished, invalid_arguments)
{
    struct disir_entry *entry = NULL;
    ASSERT_NO_SETUP_FAILURE ();

    status = disir_entry_finished (NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_entry_finished (&entry);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (EntryFinished, remove_last_element)
{
    struct disir_entry *current;
    struct disir_entry *last;
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_entries (instance, "test", &queue);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (queue->prev != NULL);
    ASSERT_TRUE (queue->prev->next == NULL);

    current = queue->prev;
    last = queue->prev;
    status = disir_entry_finished (&current);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (queue->prev != last);
}

TEST_F (EntryFinished, remove_first_element)
{
    struct disir_entry *current;
    struct disir_entry *next;
    struct disir_entry *first;
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_entries (instance, "test", &queue);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (queue->prev != NULL);
    ASSERT_TRUE (queue->next->prev == queue);

    // Safe the queue pointer for deallocation
    first = current = queue;
    queue = queue->next;
    next = first->next;

    status = disir_entry_finished (&current);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (next->prev != first);
}

TEST_F (EntryFinished, remove_all_elements)
{
    struct disir_entry *current;
    struct disir_entry *next;
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_entries (instance, "test", &queue);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (queue != NULL);

    next = queue->next;
    current = queue;
    while (1)
    {
        status = disir_entry_finished (&current);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        current = next;
        if (current == NULL)
            break;
        next = current->next;
    }
    queue = NULL;
}

TEST_F (EntryFinished, query_single_finish_single)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_query (instance, "test", "basic_keyval", &queue);
    ASSERT_STATUS (DISIR_STATUS_EXISTS, status);
    ASSERT_TRUE (queue != NULL);

    status = disir_entry_finished (&queue);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}
