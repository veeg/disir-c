#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class QueryTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = DISIR_STATUS_OK;
        collection = NULL;
        mold = NULL;
        config = NULL;
        context = NULL;
        context_keyval = NULL;
        context_mold = NULL;

        status = disir_mold_input (instance, "test", "basic_keyval", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);

        context_mold = dc_mold_getcontext (mold);
        ASSERT_TRUE (context_mold != NULL);

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_mold)
        {
            dc_putcontext (&context_mold);
        }

        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;

    struct disir_mold   *mold;
    struct disir_config *config;

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
