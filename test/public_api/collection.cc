
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class CollectionTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        // Use mold as a testbed to add create/add number of keyvals
        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = DISIR_STATUS_OK;
        collection = dc_collection_create();

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (collection != NULL)
        {
            dc_collection_finished (&collection);
        }

        if (context_mold)
        {
            dc_destroy (&context_mold);
        }

        DisirLogCurrentTestExit ();
    }

public:
    struct disir_context *context_mold;
    struct disir_context *context;
    enum disir_status status;
    struct disir_collection *collection;
    int32_t size;
    int i;
};

TEST_F (CollectionTest, create)
{
    // Assert that the collection was properly created
    ASSERT_TRUE (collection != NULL);

    // Assert that we can properly finish the collection
    status = dc_collection_finished (&collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CollectionTest, finished)
{
    // Assert that we can properly finish the collection
    status = dc_collection_finished (&collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, collection);
}

TEST_F (CollectionTest, finished_null_argument)
{
    // Assert that we can properly finish the collection
    status = dc_collection_finished (NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (CollectionTest, size_null_argument)
{
    size = dc_collection_size (NULL);
    ASSERT_EQ (0, size);
}

TEST_F (CollectionTest, size_empty_collection)
{
    size = dc_collection_size (collection);
    ASSERT_EQ (0, size);
}

//! Add a few entries and assert that collection contains all of them
//! Destroy a few entries so those are purged from the collection
//! Add a few more, assert size.
TEST_F (CollectionTest, push_context)
{
    struct disir_context *contexts[20];

    for (i =  0; i < 10; i++)
    {
        // QUESTION: Can we safely only allocate a context, without allocating the
        // corresponding value? :s
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_collection_push_context (collection, contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    // Destroy a few of them
    dc_destroy (&contexts[2]);
    dc_destroy (&contexts[6]);
    dc_destroy (&contexts[9]);

    // Add 5 more
    for (; i < 15; i++)
    {
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_collection_push_context (collection, contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    size = dc_collection_size (collection);
    ASSERT_EQ (12, size);

    // Cleanup allocated contexts (collection has the final refcount
    for (i = 0; i < 15; i++)
    {
        if (contexts[i] != NULL)
        {
            dc_destroy (&contexts[i]);
        }
    }
}

TEST_F (CollectionTest, next)
{
    struct disir_context *contexts[100];
    struct disir_context *current;

    // Insert 100 entries
    for (i = 0; i < 100; i++)
    {

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_collection_push_context (collection, contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    // Iterate through all of them
    for (i = 0; i < 100; i++)
    {
        status = dc_collection_next (collection, &current);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (current == contexts[i]);
    }

    // Iterator should be exhausted now
    status = dc_collection_next (collection, &current);
    EXPECT_STATUS (DISIR_STATUS_EXHAUSTED, status);

    // Reset iterator
    status = dc_collection_reset (collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Iterate 25 - destroy 10 before and 10 after
    // Iterate til end.
    // The internal iterator index should still return the correct entry,
    for (i = 0; i < 25; i++)
    {
        status = dc_collection_next (collection, &current);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (current == contexts[i]);
    }
    for (i = 0; i <= 20; i += 2)
    {
        dc_destroy (&contexts[i]);
    }
    for (i = 50; i < 80; i += 3)
    {
        dc_destroy (&contexts[i]);
    }

    for (i = 25; i < 100; i++)
    {
        if (contexts[i] == NULL)
        {
            continue;
        }

        status = dc_collection_next (collection, &current);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (current == contexts[i]);
    }


    // Cleanup allocated contexts (collection has the final refcount
    for (i = 0; i < 100; i++)
    {
        if (contexts[i] != NULL)
        {
            dc_destroy (&contexts[i]);
        }
    }
}

TEST_F (CollectionTest, push_context_until_internal_resize)
{
    struct disir_context *contexts[255];

    // XXX: Should dynamically retrieve the internal resize threshold.

    for (i = 0; i < 255; i++)
    {
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (contexts[i] != NULL);

        status = dc_collection_push_context (collection, contexts[i]);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        size = dc_collection_size (collection);
        ASSERT_EQ (i + 1, size);
    }

    for (i = 0; i < 255; i++)
    {
        status = dc_destroy (&contexts[i]);
        // status is not OK if the context contains no value.
        size = dc_collection_size (collection);
        ASSERT_EQ (255 - (i + 1), size);
    }
}

