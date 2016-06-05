#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test config API with empty mold.
class ContextConfigTest : public testing::Test
{
    void SetUp()
    {
        context = NULL;
        context_config = NULL;
        context_mold = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_EQ (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_mold != NULL);
        status = dc_mold_finalize (&context_mold, &mold);
        ASSERT_EQ (DISIR_STATUS_OK, status);

        status = dc_config_begin (mold, &context_config);
    }

    void TearDown()
    {

        if (context_config)
        {
            dc_destroy (&context_config);
        }

        if (mold)
        {
            disir_mold_finished (&mold);
        }

    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_config;
    struct disir_context *context_mold;
    struct disir_config *config;
    struct disir_mold *mold;
};

TEST_F (ContextConfigTest, context_type)
{
    ASSERT_EQ (DISIR_CONTEXT_CONFIG, dc_context_type (context_config));
}

TEST_F (ContextConfigTest, context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context_config), "CONFIG");
}

TEST_F (ContextConfigTest, config_begin)
{
    ASSERT_EQ (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_config != NULL);
}

TEST_F (ContextConfigTest, config_begin_invalid_arguments)
{
    status = dc_config_begin (NULL, NULL);
    ASSERT_EQ (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_config_begin (NULL, &context);
    ASSERT_EQ (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_config_begin (mold, NULL);
    ASSERT_EQ (DISIR_STATUS_INVALID_ARGUMENT, status);
}

