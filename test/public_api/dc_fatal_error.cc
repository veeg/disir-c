#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


TEST (FatalErrorTest, keyval_fatal_error_preserved_after_finalize)
{
    enum disir_status status;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;

    status = dc_mold_begin (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_keyval, "docdoc", strlen ("docdoc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "namename", strlen ("namename"));

    // Set the error
    status = dc_fatal_error (context_keyval, "this is wrong");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (context_keyval != NULL);

    // The context should still be invalid
    ASSERT_STREQ ("this is wrong", dc_context_error (context_keyval));

    dc_destroy (&context_mold);
    dc_destroy (&context_keyval);
}

TEST (FatalErrorTest, keyval_finalized_fatal_error_wrong_state)
{
    enum disir_status status;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;

    status = dc_mold_begin (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_keyval_boolean (context_mold, "test", 0, "doc", NULL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Set the error
    status = dc_fatal_error (context_keyval, "this is wrong");
    ASSERT_STATUS (DISIR_STATUS_CONTEXT_IN_WRONG_STATE, status);

    dc_putcontext (&context_keyval);
    dc_destroy (&context_mold);
}

