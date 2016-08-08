#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class MoldSectionTest : public testing::DisirTestWrapper
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

        DisirLogCurrentTest ("SetUp completed - TestBody:");
    }

    void TearDown()
    {
        DisirLogCurrentTest ("TestBody completed - TearDown:");

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


TEST_F (MoldSectionTest, context_type)
{
    ASSERT_EQ (DISIR_CONTEXT_SECTION, dc_context_type (context_section));
}

TEST_F (MoldSectionTest, context_type_string)
{
    ASSERT_STREQ ("SECTION", dc_context_type_string (context_section));
}

TEST_F (MoldSectionTest, finalizing_without_name_shall_fail)
{
    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    EXPECT_STREQ ("Missing name component for section.", dc_context_error (context_section));
}

TEST_F (MoldSectionTest, finalize_empty_with_name_shall_succeed)
{
    status = dc_set_name (context_section, "test_name", strlen ("test_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldSectionTest, set_name_on_mold)
{
    status = dc_set_name (context_section, "test_name", strlen ("test_name"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldSectionTest, begin_keyval_shall_succeed)
{
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldSectionTest, begin_section_shall_succeed)
{
    status = dc_begin (context_section, DISIR_CONTEXT_SECTION, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldSectionTest, begin_default_shall_fail)
{
    status = dc_begin (context_section, DISIR_CONTEXT_DEFAULT, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (MoldSectionTest, begin_free_text_shall_fail)
{
    status = dc_begin (context_section, DISIR_CONTEXT_FREE_TEXT, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (MoldSectionTest, begin_mold_shall_fail)
{
    status = dc_begin (context_section, DISIR_CONTEXT_MOLD, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (MoldSectionTest, begin_config_shall_fail)
{
    status = dc_begin (context_section, DISIR_CONTEXT_CONFIG, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (MoldSectionTest, add_keyval)
{
    status = dc_add_keyval_string (context_section, "test_key", "test val", "doc st", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldSectionTest, introduced)
{
    struct semantic_version input;
    struct semantic_version output;

    input.sv_patch = 1;
    input.sv_minor = 1;
    input.sv_major = 1;

    status = dc_add_introduced (context_section, &input);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_introduced (context_section, &output);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (0, dc_semantic_version_compare (&input, &output));
}

