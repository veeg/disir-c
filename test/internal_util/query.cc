#include <gtest/gtest.h>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "context_private.h"
#include "query_private.h"
}

#include "test_helper.h"

class QueryInternal : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        memset (name, 0, 2048);
        memset (resolved, 0, 2048);
        memset (child_name, 0, 256);

        status = disir_mold_read (instance, "test", "config_query_permutations", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_generate_config_from_mold (mold, NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        context_config = dc_config_getcontext (config);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (parent)
        {
            dc_putcontext (&parent);
        }
        if (ancestor)
        {
            dc_putcontext (&ancestor);
        }
        if (context_config)
        {
            dc_putcontext (&context_config);
        }
        if (config)
        {
            disir_config_finished (&config);
        }
        if (mold)
        {
            disir_mold_finished (&mold);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    char name[2048];
    char resolved[2048];
    char child_name[256];
    int child_index = 0;

    char *next = NULL;
    int index = 0;
    va_list args;

    struct disir_config *config = NULL;
    struct disir_mold *mold = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *ancestor = NULL;
    struct disir_context *parent = NULL;
};

TEST_F (QueryInternal, ensure_ancestors_root_keyval)
{
    status = dx_query_ensure_ancestors (context_config, "root", args,
                                        &ancestor, &parent, child_name, &child_index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_TRUE(ancestor == NULL);
    EXPECT_TRUE(parent == context_config);
    EXPECT_STREQ("root", child_name);
    EXPECT_EQ(0, child_index);

    dc_putcontext (&parent);

    child_name[0] = '\0';
    status = dx_query_ensure_ancestors (context_config, "xxx@4", args,
                                        &ancestor, &parent, child_name, &child_index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ("xxx", child_name);
    EXPECT_EQ(4, child_index);
}

TEST_F (QueryInternal, ensure_ancestors_immediate_parent_section_exist)
{
    status = dx_query_ensure_ancestors (context_config, "first@0.key_string", args,
                                        &ancestor, &parent, child_name, &child_index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_TRUE(ancestor == NULL);
    EXPECT_TRUE(parent != context_config);
    EXPECT_STREQ("key_string", child_name);
    EXPECT_EQ(0, child_index);

    // QUESTION: Should we assert the name of parent?
}

TEST_F (QueryInternal, ensure_ancestors_immediate_parent_section_doest_not_exist)
{
    status = dx_query_ensure_ancestors (context_config, "first@2.key_string", args,
                                        &ancestor, &parent, child_name, &child_index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_TRUE(ancestor != NULL);
    EXPECT_TRUE(parent != context_config);
    EXPECT_TRUE(ancestor == parent);

    status = dc_finalize (&ancestor);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    // QUESTION: Should we assert the name of parent?
}

TEST_F (QueryInternal, ensure_ancestors_immediate_parent_section_index_out_of_bounds)
{
    status = dx_query_ensure_ancestors (context_config, "first@3.key_string", args,
                                        &ancestor, &parent, child_name, &child_index);
    EXPECT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
    EXPECT_STREQ ("accessing index first@3 is out of range of existing number of elements 2," \
                  " can only access index that is one greater",
                  dc_context_error (context_config));
}

TEST_F (QueryInternal, ensure_ancestors_section_does_not_have_mold)
{
    status = dx_query_ensure_ancestors (context_config, "not_exist@0.key_string", args,
                                        &ancestor, &parent, child_name, &child_index);
    EXPECT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    EXPECT_STREQ ("section not_exist@0 does not exist",
                  dc_context_error (context_config));
}

TEST_F (QueryInternal, ensure_ancestors_section_parent_is_keyval_does_not_exist)
{
    status = dx_query_ensure_ancestors (context_config, "first.key_string@1.hola", args,
                                        &ancestor, &parent, child_name, &child_index);
    EXPECT_STATUS (DISIR_STATUS_CONFLICT, status);
    EXPECT_STREQ ("expected first.key_string@1 to be a section, is keyval",
                  dc_context_error (context_config));
}

TEST_F (QueryInternal, ensure_ancestors_section_parent_is_keyval_exists)
{
    status = dx_query_ensure_ancestors (context_config, "root.hola", args,
                                        &ancestor, &parent, child_name, &child_index);
    EXPECT_STATUS (DISIR_STATUS_CONFLICT, status);
    EXPECT_STREQ ("expected root to be a section, is keyval",
                  dc_context_error (context_config));
}
