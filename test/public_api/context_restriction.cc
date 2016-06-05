

#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

// PRVIATE API
extern "C" {
#include "context_private.h"
}

#include "test_helper.h"


// Test mold API with empty mold.
class ContextRestrictionTest : public testing::Test
{
    void SetUp()
    {
        context = dx_context_create (DISIR_CONTEXT_RESTRICTION);
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

// TODO: Remove DISABLED_ when implementing restrictions
TEST_F (ContextRestrictionTest, DISABLED_context_type)
{
    ASSERT_EQ (dc_context_type (context), DISIR_CONTEXT_RESTRICTION);
}

// TODO: Remove DISABLED_ when implementing restrictions
TEST_F (ContextRestrictionTest, DISABLED_context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context), "RESTRICTION");
}

