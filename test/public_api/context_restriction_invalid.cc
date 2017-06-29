
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"

class ContextRestrictionInvalidTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        status = dc_mold_begin (&context_mold);
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
        if (context_section)
        {
            dc_destroy (&context_section);
        }
        if (context_restriction)
        {
            dc_destroy (&context_restriction);
        }
    }
public:
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_section = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_restriction = NULL;
};


TEST_F (ContextRestrictionInvalidTest, parent_keyval_invalid_on_no_type)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_default_string (context_keyval, "keyval", strlen ("keyval"), NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_keyval, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (ContextRestrictionInvalidTest, parent_section_invalid_on_no_type)
{
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section", strlen ("section"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_section, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_restriction);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

