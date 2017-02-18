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

