// PUBLIC API
#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions:
//  dc_get_value
//
class DcGetValueTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_read (instance, "test", "basic_keyval", NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_config = dc_config_getcontext (config);

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
    int32_t output_size;
    char buffer[1024];
    int buffer_size = 1024;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_config *config = NULL;
    struct disir_context *context_resolved = NULL;
};


TEST_F (DcGetValueTest, invalid_arguments)
{
    dc_find_element (context_config, "key_integer", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_value (NULL, 0, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_value (context_keyval, 0, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_value (context_config, 0, buffer, NULL);
    EXPECT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (DcGetValueTest, config_keyval_string)
{
    dc_find_element (context_config, "key_string", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Without output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("string_value", buffer);

    // With output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (strlen("string_value"), output_size);
}

TEST_F (DcGetValueTest, config_keyval_integer)
{
    dc_find_element (context_config, "key_integer", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Without output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("42", buffer);

    // With output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (strlen("42"), output_size);
}

TEST_F (DcGetValueTest, config_keyval_float)
{
    dc_find_element (context_config, "key_float", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Without output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("3.140000", buffer);

    // With output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (strlen("3.140000"), output_size);
}

TEST_F (DcGetValueTest, config_keyval_boolean)
{
    dc_find_element (context_config, "key_boolean", 0, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Without output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ ("True", buffer);

    // With output_size
    status = dc_get_value (context_keyval, buffer_size, buffer, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (strlen("True"), output_size);
}
