#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class ContextDocumentationTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        context = NULL;
        context_mold = NULL;
        context_section = NULL;

        // TODO: Setup mold, keyval, config, keyval
        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_mold, DISIR_CONTEXT_DOCUMENTATION, &context);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context != NULL);

        status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_section != NULL);
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
        if (context_section)
        {
            // this will assert that dc_destroy
            // on a documentation context works
            status = dc_destroy (&context_section);
            ASSERT_STATUS  (DISIR_STATUS_OK, status);
        }

    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_section;
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

TEST_F (ContextDocumentationTest, section_add_and_get_documentation)
{
    const char doc[] = "Documentation string";
    const char *out_doc;
    int32_t size;

    status = dc_add_documentation (context_section, doc, strlen (doc));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_documentation (context_section, NULL, &out_doc, &size);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ (doc, out_doc);
    ASSERT_EQ (strlen (doc), size);
}

