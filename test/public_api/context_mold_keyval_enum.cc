// PUBLIC API
#include <disir/disir.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions when the disir_context is a DISIR_CONTEXT_KEYVAL
// of disir_value_type DISIR_VALUE_TYPE_ENUM, where the disir_value_type matters.
//      dc_set_value_enum
//      dc_get_value_enum
class ContextMoldKeyvalEnumTest : public testing::DisirTestWrapper
{
protected:
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_ENUM);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_set_name (context_keyval, keyval_name, strlen (keyval_name));
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

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

        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status;
    const char *keyval_name = "test_keyval_name";
    const char *testvalue = "test string value";
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
};


TEST_F (ContextMoldKeyvalEnumTest, dc_set_value_enum_shall_fail_due_to_wrong_type)
{

    ASSERT_NO_SETUP_FAILURE();

    status = dc_set_value_enum (context_keyval, testvalue, strlen (testvalue));
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (ContextMoldKeyvalEnumTest, dc_get_value_enum_shall_fail_due_to_wrong_type)
{
    const char *output;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_get_value_enum (context_keyval, &output, NULL);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

