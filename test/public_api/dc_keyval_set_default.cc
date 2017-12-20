// PUBLIC API
#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions:
//  dc_keyval_set_default
//
class DcKeyvalSetDefault : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_mold_read (instance, "test", "basic_keyval", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_mold = dc_mold_getcontext (mold);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (config)
        {
            disir_config_finished (&config);
        }
        if (context_config)
        {
            dc_putcontext (&context_config);
        }
        if (context_keyval)
        {
            dc_putcontext (&context_keyval);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_mold = NULL;
    struct disir_config *config = NULL;
    struct disir_mold *mold = NULL;
};


TEST_F (DcKeyvalSetDefault, invalid_arguments)
{
    status = dc_keyval_set_default (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    // We only expect context of type KEYVAL
    status = dc_keyval_set_default (context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    // We only expect context of type KEYVAL whose root is CONFIG
    status = dc_find_element (context_mold, "key_string", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_keyval_set_default (context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
    dc_putcontext(&context_keyval);
}

TEST_F (DcKeyvalSetDefault, no_name_set)
{
    status = dc_config_begin (mold, &context_config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // This should fail horribly. We do not have a mold equivalent
    status = dc_keyval_set_default (context_keyval, NULL);
    EXPECT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
}

TEST_F (DcKeyvalSetDefault, success_default_version)
{
    status = dc_config_begin (mold, &context_config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "key_string", strlen("key_string"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_keyval_set_default (context_keyval, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    const char* value = NULL;
    status = dc_get_value_string (context_keyval, &value, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("string_value", value);
}
