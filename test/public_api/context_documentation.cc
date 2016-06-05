#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class ContextDocumentationTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        // TODO: Setup mold, keyval, config, keyval
        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_mold, DISIR_CONTEXT_DOCUMENTATION, &context);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context != NULL);
    }

    void TearDown()
    {
        if (context)
        {
            dc_destroy (&context);
        }
        if (context_mold)
        {
            dc_destroy (&context_mold);
        }

    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
};


TEST_F (ContextDocumentationTest, context_type)
{
    ASSERT_EQ (dc_context_type (context), DISIR_CONTEXT_DOCUMENTATION);
}

TEST_F (ContextDocumentationTest, context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context), "DOCUMENTATION");
}

TEST_F (ContextDocumentationTest, add_documentation_invalid_arguments_shall_fail)
{
    const char doc[] = "A documentation string";

    status = dc_add_documentation (NULL, NULL, 0);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_documentation (context_mold, NULL, 0);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_documentation (NULL, doc, 0);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_documentation (context_mold, doc, 0);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_documentation (context_mold, doc, -87);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

