
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class ContextRestrictionMoldInclusiveRestrictionTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_keyval, DISIR_CONTEXT_RESTRICTION, &context_restriction1);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_keyval, DISIR_CONTEXT_RESTRICTION, &context_restriction2);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        if (context_mold)
        {
            dc_destroy (&context_mold);
        }
        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }
        if (context_restriction1)
        {
            dc_destroy (&context_restriction1);
        }
        if (context_restriction2)
        {
            dc_destroy (&context_restriction2);
        }
    }

public:
    enum disir_status status;
    const char *error;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_restriction1 = NULL;
    struct disir_context *context_restriction2 = NULL;
};


TEST_F (ContextRestrictionMoldInclusiveRestrictionTest, entry_min_finalize)
{
    status = dc_set_restriction_type (context_restriction1, DISIR_RESTRICTION_INC_ENTRY_MIN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextRestrictionMoldInclusiveRestrictionTest,
        entry_min_two_second_equal_semver_shall_fail)
{
    // Setup first
    status = dc_set_restriction_type (context_restriction1, DISIR_RESTRICTION_INC_ENTRY_MIN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_restriction1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_restriction_type (context_restriction2, DISIR_RESTRICTION_INC_ENTRY_MIN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction2);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (NULL != context_restriction2);
    error = dc_context_error (context_restriction2);
    ASSERT_STREQ ("introduced version conflicts with another MINIMUM_ENTRIES restriction.", error);

    // cleanup
    dc_putcontext (&context_restriction2);
}

TEST_F (ContextRestrictionMoldInclusiveRestrictionTest, entry_max_finalize)
{
    status = dc_set_restriction_type (context_restriction1, DISIR_RESTRICTION_INC_ENTRY_MAX);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextRestrictionMoldInclusiveRestrictionTest,
        entry_max_two_second_equal_semver_shall_fail)
{
    // Setup first
    status = dc_set_restriction_type (context_restriction1, DISIR_RESTRICTION_INC_ENTRY_MAX);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_restriction1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_restriction_type (context_restriction2, DISIR_RESTRICTION_INC_ENTRY_MAX);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction2);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (NULL != context_restriction2);
    error = dc_context_error (context_restriction2);
    ASSERT_STREQ ("introduced version conflicts with another MAXIMUM_ENTRIES restriction.", error);

    // cleanup
    dc_putcontext (&context_restriction2);
}

