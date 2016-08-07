
// PUBLIC API
#include <disir/disir.h>


#include "test_helper.h"


// Test mold API with empty mold.
class ContextRestrictionUtilTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogTestBodyEnter ();

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_keyval, DISIR_CONTEXT_RESTRICTION, &context_restriction);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (context_mold)
        {
            dc_destroy (&context_mold);
        }
        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }
        if (context_restriction)
        {
            dc_destroy (&context_restriction);
        }
    }

public:
    enum disir_status status;
    const char *name;
    enum disir_restriction_type type = DISIR_RESTRICTION_UNKNOWN;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_restriction = NULL;
};


TEST_F (ContextRestrictionUtilTest, restriction_enum_string_inclusive_entry_min)
{
    name = dc_restriction_enum_string (DISIR_RESTRICTION_INC_ENTRY_MIN);
    ASSERT_STREQ ("MINIMUM_ENTRIES", name);
}

TEST_F (ContextRestrictionUtilTest, restriction_enum_string_inclusive_entry_max)
{
    name = dc_restriction_enum_string (DISIR_RESTRICTION_INC_ENTRY_MAX);
    ASSERT_STREQ ("MAXIMUM_ENTRIES", name);
}

TEST_F (ContextRestrictionUtilTest, restriction_enum_string_exclusive_value_enum)
{
    name = dc_restriction_enum_string (DISIR_RESTRICTION_EXC_VALUE_ENUM);
    ASSERT_STREQ ("ENUM", name);
}

TEST_F (ContextRestrictionUtilTest, restriction_enum_string_exclusive_value_range)
{
    name = dc_restriction_enum_string (DISIR_RESTRICTION_EXC_VALUE_RANGE);
    ASSERT_STREQ ("RANGE", name);
}

TEST_F (ContextRestrictionUtilTest, restriction_enum_string_exclusive_value_numeric)
{
    name = dc_restriction_enum_string (DISIR_RESTRICTION_EXC_VALUE_NUMERIC);
    ASSERT_STREQ ("NUMERIC", name);
}

TEST_F (ContextRestrictionUtilTest, restriction_context_string_null_argument)
{
    name = dc_restriction_context_string (NULL);
    ASSERT_STREQ ("INVALID", name);
}

TEST_F (ContextRestrictionUtilTest, get_restriction_type_null_arguments)
{
    status = dc_get_restriction_type (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_restriction_type (NULL, &type);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_restriction_type (context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, set_restriction_type_null_argument)
{
    status = dc_set_restriction_type (NULL, type);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, set_restriction_type_finalized_shall_fail)
{
    struct disir_context *context;

    // setup
    status = dc_set_restriction_type (context_restriction, DISIR_RESTRICTION_INC_ENTRY_MIN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    // Cheat by holding on to a context pointer.
    context = context_restriction;
    status = dc_finalize (&context_restriction);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_restriction_type (context, type);
    EXPECT_STATUS (DISIR_STATUS_CONTEXT_IN_WRONG_STATE, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_get_string_null_arguments)
{
    const char *value;

    status = dc_restriction_get_string (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_string (context_restriction, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_string (NULL, &value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_set_string_null_arguments)
{
    const char *value = "test string";

    status = dc_restriction_set_string (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_set_string (context_restriction, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_set_string (NULL, value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_get_range_null_arguments)
{
    double value;

    status = dc_restriction_get_range (NULL, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (context_restriction, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (context_restriction, &value, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (context_restriction, NULL, &value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (NULL, &value, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (NULL, NULL, &value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_range (NULL, &value, &value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_set_range_null_argument)
{
    double value;

    value = 3.14;

    status = dc_restriction_set_range (NULL, value, value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_get_numeric_null_arguments)
{
    double value;

    status = dc_restriction_get_numeric (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_numeric (context_restriction, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_get_numeric (NULL, &value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ContextRestrictionUtilTest, restriction_set_numeric_null_argument)
{
    double value;

    value = 13.37;

    status = dc_restriction_set_numeric (NULL, value);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

