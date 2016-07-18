
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test public  disir_default API on a constructing mold with single string keyval
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
    struct disir_context *context_default;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
};

class ContextDefaultEmptyTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        context_default = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

    }

    void TearDown()
    {
        if (context_default)
        {
            dc_destroy (&context_default);
            ASSERT_STATUS (status, DISIR_STATUS_OK);
            ASSERT_EQ (NULL, context_default);
        }
        if (context_keyval)
        {
            status = dc_destroy (&context_keyval);
            ASSERT_STATUS (status, DISIR_STATUS_OK);
            ASSERT_EQ (NULL, context_keyval);

        }
        if (context_mold)
        {
            status = dc_destroy (&context_mold);
            ASSERT_STATUS (status, DISIR_STATUS_OK);
            ASSERT_EQ (NULL, context_mold);
        }

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_default;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
};

TEST_F (ContextDefaultEmptyTest, begin_keyval_without_type_shall_fail)
{
    status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Cannot add a default entry to a keyval who has not yet defined a value type.",
                  dc_context_error (context_keyval));
}

TEST_F (ContextDefaultEmptyTest, begin_keyval_with_value_shall_succeed)
{
    // Setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (status, DISIR_STATUS_OK);

    status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
    ASSERT_STATUS (status, DISIR_STATUS_OK);
}

// No finalized entries - Parent context(s) has no children
// But each child is linked to its potential parent.
// Destroy in in-logical order
TEST_F (ContextDefaultEmptyTest, destroy_parent_before_default)
{
    // Setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Destroy mold first, then keyval, then default.
    status = dc_destroy (&context_mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, context_mold);

    status = dc_destroy (&context_default);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, context_default);

    dc_destroy (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, context_keyval);
}

TEST_F (ContextDefaultTest, context_type)
{
    ASSERT_EQ (DISIR_CONTEXT_DEFAULT, dc_context_type (context_default));
}

TEST_F (ContextDefaultTest, context_type_string)
{
    ASSERT_STREQ ("DEFAULT", dc_context_type_string (context_default));
}

TEST_F (ContextDefaultEmptyTest, add_default_integer_invalid_argument)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_INTEGER);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default_integer (NULL, 42, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_generic_on_integer_invalid_arguments_shall_fail)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_INTEGER);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default (context_keyval, NULL, 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_default (context_keyval, "", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_default (context_keyval, "", 1000, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_default (context_keyval, "string start 5123", 1000, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_generic_on_integer_valid_input_shall_succeed)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_INTEGER);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default (context_keyval, "1234", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}


TEST_F (ContextDefaultEmptyTest, add_default_string_invalid_argument)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default_string (NULL, "10", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_boolean_invalid_argument)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_BOOLEAN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default_boolean (NULL, 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_float_invalid_argument)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_FLOAT);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default_float (NULL, 3.14, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_generic_on_float_invalid_arguments_shall_fail)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_FLOAT);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default (context_keyval, NULL, 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_default (context_keyval, "", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_add_default (context_keyval, "string start 5123", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextDefaultEmptyTest, add_default_generic_on_float_valid_input_shall_succeed)
{
    // setup
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_INTEGER);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default (context_keyval, "3.14", 0, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextDefaultTest, set_and_get_value_string)
{
    char val[] = "testval";
    const char *output;
    int32_t size;

    status = dc_set_value_string (context_default, val, strlen (val));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_value_string (context_default, &output, &size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (strlen (val), size);
}

TEST_F (ContextDefaultTest, set_value_int_on_type_string_shall_fail)
{
    status = dc_set_value_integer (context_default, 42);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}

TEST_F (ContextDefaultTest, set_value_boolean_on_type_string_shall_fail)
{
    status = dc_set_value_boolean (context_default, 1);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}

TEST_F (ContextDefaultTest, set_value_float_on_type_string_shall_fail)
{
    status = dc_set_value_float (context_default, 3.14);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}


