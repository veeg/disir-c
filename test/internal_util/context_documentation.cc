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
class ContextDefaultTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (status, DISIR_STATUS_OK);

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (status, DISIR_STATUS_OK);

        status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
        ASSERT_STATUS (status, DISIR_STATUS_OK);
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

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *invalid;
    struct disir_context *context_default;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
    struct disir_default *def;
};


