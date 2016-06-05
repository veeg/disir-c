#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

// PRIVATE API
extern "C" {
#include "context_private.h"
}

#include "test_helper.h"



// Test config API with empty mold.
class ContextGenericTest : public testing::Test
{
    void SetUp()
    {
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
    }

public:
    enum disir_status status;
    struct disir_context *context;
};

TEST_F (ContextGenericTest, context_type_invalid_context)
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

TEST_F (ContextGenericTest, context_type_string_invalid_context)
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

