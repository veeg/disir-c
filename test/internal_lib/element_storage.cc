#include <gtest/gtest.h>
#include <list>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "context_private.h"
#include "element_storage.h"
}

#include "test_helper.h"

// Global constants used in tests
const char *keyval_names[] = {
    "carfight",
    "snitch",
    "powertools",
    "riverbed",
    "rocks",
    "aspen",
    "thundra",
    "hellfire",
    "borean",
    "durotar",
    "thrall",
    "milk",
    "porridge",
    "flowers",
};

#define KEYVAL_NUMENTRIES (sizeof (keyval_names) / sizeof (const char *))


class ElementStorageEmptyTest : public testing::DisirTestWrapper
{
protected:
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        context = NULL;

        storage = dx_element_storage_create ();
        ASSERT_TRUE (storage != NULL);
    }

    void TearDown()
    {
        status = dx_element_storage_destroy (&storage);
        ASSERT_EQ (NULL, storage);

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_element_storage *storage;
    struct disir_context *context;
    struct disir_collection *collection;
};

class ElementStoragePopulatedTest : public ElementStorageEmptyTest
{
    void SetUp()
    {
        ElementStorageEmptyTest::SetUp();

        collection = NULL;

        unsigned int i;
        unsigned int j;
        const char *key;

        // Add X number of struct disir_context *context to storage, by name.
        for (j = 0; j < 3; j++)
        {
            for (i = 0; i < KEYVAL_NUMENTRIES; i++)
            {
                key = keyval_names[i];

                context = dx_context_create (DISIR_CONTEXT_KEYVAL);
                ASSERT_TRUE (context != NULL);

                status = dx_element_storage_add (storage, key, context);
                ASSERT_STATUS (DISIR_STATUS_OK, status);

                list.push_back (context);
            }
        }
    }

    void TearDown()
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            context = (*it);
            dx_context_decref (&context);
        }

        if (collection)
        {
            status = dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        ElementStorageEmptyTest::TearDown();
    }
public:
    std::list<struct disir_context *>   list;
};


TEST_F (ElementStorageEmptyTest, destroy_invalid_argument)
{
    struct disir_element_storage *null_storage = NULL;

    status = dx_element_storage_destroy (NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dx_element_storage_destroy (&null_storage);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ElementStorageEmptyTest, add)
{
    unsigned int i;
    const char *key;

    // Add X number of struct disir_context *context to storage, by name.
    for (i = 0; i < KEYVAL_NUMENTRIES; i++)
    {
        key = keyval_names[i];

        context = dx_context_create (DISIR_CONTEXT_KEYVAL);
        ASSERT_TRUE (context != NULL);

        status = dx_element_storage_add (storage, key, context);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        // Ref count should be 2 when inserted into element storage
        ASSERT_EQ (2, context->cx_refcount);

        // Put back the context - we are finished with it.
        dx_context_decref (&context);
        ASSERT_EQ (1, context->cx_refcount);
    }

    // Destroying storage shall free the decref the context, which will destroy it.
    // Destroying is done in TearDown()
    // Hard to verify that memory has been free'd. Rely on memcheck tools.
}

TEST_F (ElementStorageEmptyTest, add_with_duplicate_keys_shall_succeed)
{
    unsigned int i;
    unsigned int j;
    const char *key;

    // Add X number of struct disir_context *context to storage, by name.
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < KEYVAL_NUMENTRIES; i++)
        {
            key = keyval_names[i];

            context = dx_context_create (DISIR_CONTEXT_KEYVAL);
            ASSERT_TRUE (context != NULL);

            status = dx_element_storage_add (storage, key, context);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
            // Ref count should be 2 when inserted into element storage
            ASSERT_EQ (2, context->cx_refcount);

            // Put back the context - we are finished with it.
            dx_context_decref (&context);
            ASSERT_EQ (1, context->cx_refcount);
        }
    }

    // Destroying storage shall free the decref the context, which will destroy it.
    // Destroying is done in TearDown()
    // Hard to verify that memory has been free'd. Rely on memcheck tools.
}

TEST_F (ElementStorageEmptyTest, get_all_invalid_argument_shall_fail)
{
    status = dx_element_storage_get_all (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dx_element_storage_get_all (storage, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dx_element_storage_get_all (NULL, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ElementStorageEmptyTest, get_all_empty_storage_shall_succeed)
{
    status = dx_element_storage_get_all (storage, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    EXPECT_EQ (0, dc_collection_size (collection));
}

TEST_F (ElementStoragePopulatedTest, get_all_populated_storage_assert_size_shall_succeed)
{
    status = dx_element_storage_get_all (storage, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    EXPECT_EQ ((3 * KEYVAL_NUMENTRIES), dc_collection_size (collection));
}

TEST_F (ElementStoragePopulatedTest, get_all_collection_shall_be_in_insert_order)
{
    struct disir_context *c;

    // Setup
    status = dx_element_storage_get_all (storage, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);


    for (auto it = list.begin(); it != list.end(); ++it)
    {
        c = *it;
        status = dc_collection_next (collection, &context);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        ASSERT_EQ (c, context);
    }

    status = dc_collection_next (collection, &context);
    ASSERT_STATUS (DISIR_STATUS_EXHAUSTED, status);
}

