
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

// PRVIATE API
extern "C" {
#include "context_private.h"
#include "default.h"
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
        status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
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

// Test default API with no prerequisits
class ContextDefaultEmptyTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        context = context_default = context_mold = context_keyval = invalid = NULL;
        def = NULL;
    }

    void TearDown()
    {
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


TEST_F (ContextDefaultEmptyTest, default_begin_invalid_argument)
{
    status = dx_default_begin (NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dx_default_begin (NULL, &context_default);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dx_default_begin (context_keyval, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, default_begin_wrong_context_section)
{
    invalid = dx_context_create (DISIR_CONTEXT_SECTION);
    ASSERT_TRUE (invalid != NULL);

    status = dx_default_begin (invalid, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    dx_context_destroy (&invalid);
}

TEST_F (ContextDefaultEmptyTest, default_begin_wrong_context_config)
{
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);
    ASSERT_TRUE (invalid != NULL);

    status = dx_default_begin (invalid, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    dx_context_destroy (&invalid);
}

TEST_F (ContextDefaultEmptyTest, default_begin_wrong_context_mold)
{
    invalid = dx_context_create (DISIR_CONTEXT_MOLD);
    ASSERT_TRUE (invalid != NULL);

    status = dx_default_begin (invalid, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    dx_context_destroy (&invalid);
}

TEST_F (ContextDefaultEmptyTest, DISABLED_default_begin_wrong_context_restriction)
{
    invalid = dx_context_create (DISIR_CONTEXT_RESTRICTION);
    ASSERT_TRUE (invalid != NULL);

    status = dx_default_begin (invalid, &context);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    dx_context_destroy (&invalid);
}

TEST_F (ContextDefaultEmptyTest, default_begin_correct_context_keyval_invalid_root_context)
{
    // Root context is not defined here (NULL)
    // Expect to get wrong context with error message set
    context_keyval = dx_context_create (DISIR_CONTEXT_KEYVAL);
    ASSERT_TRUE (context_keyval != NULL);

    status = dx_default_begin (context_keyval, &context_default);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
    ASSERT_STREQ ("Cannot add default to a KEYVAL whose root is not a MOLD",
                  dc_context_error (context_keyval));

    dx_context_destroy (&context_keyval);
}

TEST_F (ContextDefaultEmptyTest, default_create_and_destroy)
{
    def = dx_default_create (NULL);
    ASSERT_TRUE (def != NULL);

    status = dx_default_destroy (&def);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (def == NULL);
}

