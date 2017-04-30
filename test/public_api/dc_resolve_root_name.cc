// PUBLIC API
#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions:
//  dc_resolve_root_name
//
class ResolveRootNameTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_read (instance, "test", "config_query_permutations",
                                    NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_config = dc_config_getcontext (config);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (config)
        {
            disir_config_finished (&config);
        }
        if (context_config)
        {
            dc_putcontext (&context_config);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_context *context_config = NULL;
    struct disir_config *config = NULL;
    struct disir_context *context_resolved = NULL;
    char *name_resolved;
};


TEST_F (ResolveRootNameTest, invalid_arguments)
{
    status = dc_resolve_root_name (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_resolve_root_name (context_config, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_resolve_root_name (NULL, &name_resolved);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ResolveRootNameTest, wrong_context_type)
{
    status = dc_resolve_root_name (context_config, &name_resolved);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (ResolveRootNameTest, single)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_query_resolve_context (context_config, "first", &context_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_resolved != NULL);

    status = dc_resolve_root_name (context_resolved, &name_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("first", name_resolved);

    // cleanup
    free (name_resolved);
    dc_putcontext (&context_resolved);
}

TEST_F (ResolveRootNameTest, simple_nested)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_query_resolve_context (context_config, "first.key_string", &context_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_resolved != NULL);

    status = dc_resolve_root_name (context_resolved, &name_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("first.key_string", name_resolved);

    // cleanup
    free (name_resolved);
    dc_putcontext (&context_resolved);
}

TEST_F (ResolveRootNameTest, first_index_1_nested)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_query_resolve_context (context_config, "first@1.key_string", &context_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_resolved != NULL);

    status = dc_resolve_root_name (context_resolved, &name_resolved);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("first@1.key_string", name_resolved);

    // cleanup
    free (name_resolved);
    dc_putcontext (&context_resolved);
}

