#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

// PRVIATE API
extern "C" {
#include "context_private.h"
#include "documentation.h"
}

#include "test_helper.h"


// Test mold API with empty mold.
class ContextDocumentationTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        context_section = NULL;
        context_keyval = NULL;
        context_mold = NULL;
        context_documentation = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (status, DISIR_STATUS_OK);

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (status, DISIR_STATUS_OK);

        status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        if (context_default)
        {
            dc_destroy (&context_default);
        }
        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }
        if (context_mold)
        {
            dc_destroy (&context_mold);
        }
        if (context_documentation)
        {
            dc_destroy (&context_documentation);
        }
        if (context_section)
        {
            dc_destroy (&context_section);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *invalid;
    struct disir_context *context_default;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
    struct disir_context *context_section;
    struct disir_context *context_documentation;
    struct disir_default *def;
};

TEST_F (ContextDocumentationTest, section_documentation)
{
    const char doc[] = "documentation string";

    status = dc_begin (context_section, DISIR_CONTEXT_DOCUMENTATION, &context_documentation);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_documentation, doc, strlen (doc));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_documentation);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (1, dx_documentation_numentries (context_section));
}

