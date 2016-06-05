#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class ContextMoldTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        collection = NULL;
        context = NULL;
        context_mold = NULL;
        context_keyval = NULL;
        mold = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_mold != NULL);
    }

    void TearDown()
    {
        if (context_keyval)
        {
            status = dc_destroy (&context_keyval);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }
        if (mold)
        {
            status = disir_mold_finished (&mold);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
            ASSERT_EQ (NULL, mold);
        }
        if (context_mold)
        {
            status = dc_destroy (&context_mold);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context)
        {
            status = dc_destroy (&context);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }
        if (collection)
        {
            status = dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
    struct disir_mold *mold;
    struct disir_collection *collection;
};

TEST_F (ContextMoldTest, context_type)
{
    ASSERT_EQ (dc_context_type (context_mold), DISIR_CONTEXT_MOLD);
}

TEST_F (ContextMoldTest, context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context_mold), "MOLD");
}

TEST_F (ContextMoldTest, mold_begin_invalid_arguments)
{
    status = dc_mold_begin (NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextMoldTest, finalize_invalid_arguments)
{
    status = dc_mold_finalize (&context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_mold_finalize (NULL, &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextMoldTest, finalize_wrong_context)
{
    // Setup
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Try to finalize mold with KEYVAL input context (which is WRONG!)
    status = dc_mold_finalize (&context, &mold);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (ContextMoldTest, finalize_shall_succeed)
{
    status = dc_mold_finalize (&context_mold, &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, context_mold);
}

TEST_F (ContextMoldTest, getcontext_invalid_argument_shall_fail)
{
    context = dc_mold_getcontext (NULL);
    ASSERT_EQ (NULL, context);
}

TEST_F (ContextMoldTest, getcontext_valid_argument_shall_succeed)
{
    status = dc_mold_finalize (&context_mold, &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (NULL, context_mold);

    context = dc_mold_getcontext (mold);
    ASSERT_TRUE (context != NULL);

    ASSERT_EQ (DISIR_CONTEXT_MOLD, dc_context_type (context));
}

TEST_F (ContextMoldTest, add_keyval_without_name_shall_fail)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_keyval != NULL);

    // Missing name
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    EXPECT_STREQ ("Missing name component for keyval.", dc_context_error (context_keyval));
}

TEST_F (ContextMoldTest, add_keyval_without_type_shall_fail)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_keyval != NULL);
    status = dc_set_name (context_keyval, "test_keyval_name", strlen ("test_keyval_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Missing type
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    EXPECT_STREQ ("Missing type component for keyval.", dc_context_error (context_keyval));
}

TEST_F (ContextMoldTest, add_keyval_without_default_shall_fail)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_keyval != NULL);
    status = dc_set_name (context_keyval, "test_keyval_name", strlen ("test_keyval_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Missing default
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    EXPECT_STREQ ("Missing default entry for keyval.", dc_context_error (context_keyval));
}

TEST_F (ContextMoldTest, add_keyval_with_name_type_default_shall_succeed)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context_keyval != NULL);
    status = dc_set_name (context_keyval, "test_keyval_name", strlen ("test_keyval_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_default_string (context_keyval, "TNT", strlen ("TNT"), NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Missing default
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextMoldTest, add_documentation)
{
    const char docstring[] = "A somewhat ponderous plan.";

    status = dc_add_documentation (context_mold, docstring, strlen (docstring));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextMoldTest, add_documentation_twice_shall_fail)
{
    const char docstring[] = "A somewhat ponderous plan.";

    status = dc_add_documentation (context_mold, docstring, strlen (docstring));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold, docstring, strlen (docstring));
    EXPECT_STATUS (DISIR_STATUS_EXISTS, status);
}

TEST_F (ContextMoldTest, get_documentation)
{
    const char docstring[] = "A somewhat ponderous plan.";
    const char *output;
    int32_t output_size;

    // Setup
    status = dc_add_documentation (context_mold, docstring, strlen (docstring));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_documentation (context_mold, NULL, &output, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ (docstring, output);
    EXPECT_EQ (strlen (docstring), output_size);
}

TEST_F (ContextMoldTest, get_version_default)
{
    struct semantic_version input;
    struct semantic_version output;

    // Setup
    input.sv_major = 1;
    input.sv_minor = 0;
    input.sv_patch = 0;
    status = dc_mold_finalize (&context_mold, &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Expect mold to have default version
    status = dc_mold_get_version (mold, &output);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (0, dc_semantic_version_compare (&input, &output));
}

TEST_F (ContextMoldTest, get_introduced)
{
    struct semantic_version input;
    struct semantic_version output;

    input.sv_major = 1;
    input.sv_minor = 0;
    input.sv_patch = 0;

    // Expect mold to have default version
    status = dc_get_introduced (context_mold, &output);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (0, dc_semantic_version_compare (&input, &output));
}

TEST_F (ContextMoldTest, add_introduced_shall_fail)
{
    struct semantic_version input;

    // Expect mold to have default version
    status = dc_add_introduced (context_mold, input);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
    EXPECT_STREQ ("Cannot add introduced version to MOLD", dc_context_error (context_mold));
}

TEST_F (ContextMoldTest, get_version_greater_than_default)
{
    struct semantic_version input;
    struct semantic_version output;

    // Setup
    input.sv_major = 2;
    input.sv_minor = 5;
    input.sv_patch = 1;

    // Setup mold with keyval whose semver is non-default
    status = dc_add_keyval_string (context_mold, "test_keyval", "defval", "keyval_doc", &input);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold, &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Expect mold to have version of added keyval
    status = dc_mold_get_version (mold, &output);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (0, dc_semantic_version_compare (&input, &output));
}

TEST_F (ContextMoldTest, get_introduced_higher_than_default)
{
    struct semantic_version input;
    struct semantic_version output;

    // Setup
    input.sv_major = 2;
    input.sv_minor = 5;
    input.sv_patch = 1;

    // Setup mold with keyval whose semver is non-default
    status = dc_add_keyval_string (context_mold, "test_keyval", "defval", "keyval_doc", &input);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Expect mold to have version of added keyval
    status = dc_get_introduced (context_mold, &output);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (0, dc_semantic_version_compare (&input, &output));
}

TEST_F (ContextMoldTest, get_elements)
{
    // TODO: Use existig test MOLD instead of adding to this myself

    // Setup mold
    status = dc_mold_begin (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_string (context_mold, "keyval1",
                                   "keyval1_value", "keyval1_doc", NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_string (context_mold, "keyval2", "keyval2_value",
                                   "keyval2_doc", NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_string (context_mold, "keyval3", "keyval3_value",
                                   "keyval3_doc", NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_string (context_mold, "keyval4", "keyval4_value",
                                   "keyval4_doc", NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold, &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (NULL, context_mold);

    context_mold = dc_mold_getcontext (mold);
    ASSERT_TRUE (context_mold != NULL);

    status = dc_get_elements (context_mold, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (4, dc_collection_size (collection));

    // TODO: Test that the order of elements are insertion order?

    // cleanup
    dc_putcontext (&context_mold);
}


