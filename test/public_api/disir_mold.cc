#include <gtest/gtest.h>

#include <disir/disir.h>

#include "test_helper.h"

//! Contains tests that test the public disir_mold_ API.
class DisirMoldTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_entry      *entries = NULL;
};

TEST_F (DisirMoldTest, entries_invalid_arguments)
{
    status = disir_mold_entries (NULL, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (instance, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (instance, "test", NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (NULL, "test", NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (NULL, "test", &entries);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (NULL, NULL, &entries);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_mold_entries (instance, NULL, &entries);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (DisirMoldTest, entries)
{
    int count = 0;
    struct disir_entry *current;
    struct disir_entry *next;

    status = disir_mold_entries (instance, "test", &entries);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (entries != NULL);

    // Iterate all entries. Count em up.
    current = entries;
    while (current != NULL)
    {
        count += 1;

        next = current->next;
        disir_entry_finished (&current);
        current = next;
    }

    ASSERT_GT (count, 0);
}

TEST_F (DisirMoldTest, valid_nested_invalid_context)
{
    struct disir_context *context_mold;
    struct disir_context *context_section;
    struct disir_collection *collection;
    struct disir_mold *mold;
    // Construct a mold with a nested invalid keyval

    status = dc_mold_begin (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section", strlen("section"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // create nested invalid key
    // This is invalid because the name contains capital characters
    status = dc_add_keyval_string (context_section, "NESTED", "value", "doc", nullptr, nullptr);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Finalize the section and mold
    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    status = dc_mold_finalize (&context_mold, &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Now, issue the operation and verify that we have the invalid element
    status = disir_mold_valid (mold, &collection);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (collection != nullptr);
    ASSERT_EQ (1, dc_collection_size(collection));

    // cleanup
    dc_collection_finished (&collection);
    dc_putcontext (&context_section);
    disir_mold_finished (&mold);
}

