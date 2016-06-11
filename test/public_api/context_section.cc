#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class ContextSectionTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        context = NULL;
        context_mold = NULL;
        context_keyval = NULL;
        context_section = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
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
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_section;
    struct disir_context *context_keyval;
};


TEST_F (ContextSectionTest, context_type)
{
    ASSERT_EQ (DISIR_CONTEXT_SECTION, dc_context_type (context_section));
}

TEST_F (ContextSectionTest, context_type_string)
{
    ASSERT_STREQ ("SECTION", dc_context_type_string (context_section));
}

TEST_F (ContextSectionTest, finalizing_without_name_shall_fail)
{
    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    EXPECT_STREQ ("Missing name component for section.", dc_context_error (context_section));
}

TEST_F (ContextSectionTest, set_name_on_mold)
{
    status = dc_set_name (context_section, "test_name", strlen ("test_name"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}
