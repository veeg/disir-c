#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class QueryTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        status = DISIR_STATUS_OK;
        collection = NULL;
        context = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_set_name (context_keyval, "test_keyval", strlen ("test_keyval"));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        if (collection != NULL)
        {
            status = dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context)
        {
            status = dc_destroy (&context);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_mold)
        {
            status = dc_destroy (&context_mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_keyval)
        {
            status = dc_destroy (&context_keyval);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;

    struct disir_collection *collection;
    int32_t size;
    int i;
};

TEST_F (QueryTest, get_elements_invalid_argument)
{
    status = dc_get_elements (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_elements (context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_elements (NULL, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (QueryTest, get_name_invalid_argument)
{
    const char *name;

    status = dc_get_name (NULL, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_name (context_keyval, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_name (NULL, &name, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}
