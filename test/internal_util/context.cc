#include <gtest/gtest.h>
#include <map>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "context_private.h"
}

#include "test_helper.h"

class ContextUtilTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        // Must supply a valid context enum to dx_context_create
        context = dx_context_create (DISIR_CONTEXT_CONFIG);
        ASSERT_TRUE (context != NULL);

    }

    void TearDown()
    {
        if (context)
        {
            dx_context_destroy (&context);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context = nullptr;
};

TEST_F (ContextUtilTest, type_check_invalid_input)
{
    struct disir_context    c;

    // input invalid c less than zero.
    c.cx_type = (enum disir_context_type) -5;
    status = CONTEXT_TYPE_CHECK (&c, DISIR_CONTEXT_CONFIG);
    ASSERT_EQ (status, DISIR_STATUS_INVALID_CONTEXT);

    // Input invalid c of way too high number
    c.cx_type = (enum disir_context_type) 98734;
    status = CONTEXT_TYPE_CHECK(&c, DISIR_CONTEXT_CONFIG);
    ASSERT_EQ (status, DISIR_STATUS_INVALID_CONTEXT);

    // Input c type of zero
    c.cx_type = (enum disir_context_type) 0;
    status = CONTEXT_TYPE_CHECK(&c, DISIR_CONTEXT_CONFIG);
    ASSERT_EQ (status, DISIR_STATUS_INVALID_CONTEXT);
}

TEST_F (ContextUtilTest, type_check_single_correct_context)
{
    struct disir_context    c;
    int i;

    // Outer loop against type to check for.
    for (i = 1; i < DISIR_CONTEXT_UNKNOWN; i++)
    {
        c.cx_type = (enum disir_context_type) i;
        status = CONTEXT_TYPE_CHECK (&c, i);
        ASSERT_EQ (status, DISIR_STATUS_OK);
    }
}

TEST_F (ContextUtilTest, type_check_single_incorrect_context)
{
    struct disir_context    c;
    int i, j;

    // Outer loop against type to check for.
    for (i = 1; i < DISIR_CONTEXT_UNKNOWN; i++)
    {
        c.cx_type = (enum disir_context_type) i;
        // Inner loop to verify that type check fails
        for (j = 1; j <= DISIR_CONTEXT_UNKNOWN; j++)
        {
            if (i == j)
                continue;
            status = CONTEXT_TYPE_CHECK (&c, j);
            ASSERT_EQ (status, DISIR_STATUS_WRONG_CONTEXT);
        }
    }
}

TEST_F (ContextUtilTest, type_check_multiple_incorrect_context)
{
    struct disir_context    c;

    // Valid c against multiple incorrect, valid cs.
    c.cx_type = DISIR_CONTEXT_MOLD;
    status = CONTEXT_TYPE_CHECK (&c, DISIR_CONTEXT_CONFIG,
                                     DISIR_CONTEXT_DEFAULT,
                                     DISIR_CONTEXT_SECTION);
    ASSERT_EQ (status, DISIR_STATUS_WRONG_CONTEXT);
}

TEST_F (ContextUtilTest, type_check_multiple_correct_context)
{
    struct disir_context    c;

    // Valid c against multiple incorrect, valid cs.
    c.cx_type = DISIR_CONTEXT_MOLD;
    status = CONTEXT_TYPE_CHECK (&c, DISIR_CONTEXT_CONFIG,
                                     DISIR_CONTEXT_DEFAULT,
                                     DISIR_CONTEXT_MOLD,
                                     DISIR_CONTEXT_SECTION);
    ASSERT_EQ (status, DISIR_STATUS_OK);
}

TEST_F (ContextUtilTest, context_type_invalid_context)
{
    //! Zero value
    context->cx_type = (enum disir_context_type) 0;
    ASSERT_EQ (dc_context_type (context), DISIR_CONTEXT_UNKNOWN);

    //! Negative value
    context->cx_type = (enum disir_context_type) -5;
    ASSERT_EQ (dc_context_type (context), DISIR_CONTEXT_UNKNOWN);

    // Random waay to high value
    context->cx_type = (enum disir_context_type) 87634;
    ASSERT_EQ (dc_context_type (context), DISIR_CONTEXT_UNKNOWN);
}

TEST_F (ContextUtilTest, context_type_string_invalid_context)
{
    //! Zero value
    context->cx_type = (enum disir_context_type) 0;
    ASSERT_STREQ (dc_context_type_string (context), "UNKNOWN");

    //! Negative value
    context->cx_type = (enum disir_context_type) -5;
    ASSERT_STREQ (dc_context_type_string (context), "UNKNOWN");

    // Random waay to high value
    context->cx_type = (enum disir_context_type) 87634;
    ASSERT_STREQ (dc_context_type_string (context), "UNKNOWN");
}

