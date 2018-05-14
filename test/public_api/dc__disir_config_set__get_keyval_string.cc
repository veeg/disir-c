// PUBLIC API
#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions:
//  disir_config_set_keyval_string
//  disir_config_get_keyval_string
//  dc_config_set_keyval_string
//  dc_config_get_keyval_string
//
// We only implement exhaustive tests for dc_config_*,
// and simply implement API interaction test for disir_config_*,
// since the underlying implementation is the same.
//
class DisirConfigSetGetKeyvalString : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_read (instance, "test", "config_query_permutations",
                                    NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_config = dc_config_getcontext (config);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (context_config)
        {
            dc_putcontext (&context_config);
        }
        if (config)
        {
            disir_config_finished (&config);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    const char *string_value = NULL;
    struct disir_context *context_config = NULL;
    struct disir_config *config = NULL;
};

TEST_F (DisirConfigSetGetKeyvalString, dc_get_existing_keyval_in_first_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_get_keyval_string (context_config, &string_value, "first.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("string_value", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_get_existing_keyval_in_first_subsection_index_specified)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_get_keyval_string (context_config, &string_value, "first@0.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("string_value", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_get_existing_keyval_in_second_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_get_keyval_string (context_config, &string_value, "first@1.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("string_value", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_get_nonexisting_keyval_in_second_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_get_keyval_string (context_config, &string_value, "first@1.key_string@1");
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_TRUE (string_value == NULL);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_get_nonexisting_keyval_in_nonexistant_section)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_get_keyval_string (context_config, &string_value, "first@2.key_string@6");
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_TRUE (string_value == NULL);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_set_existing_key_in_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@1.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_get_keyval_string (context_config, &string_value, "first@1.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("bloody", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_set_nonexisting_key_in_subsection_within_range)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@1.key_string@1");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_get_keyval_string (context_config, &string_value, "first@1.key_string@1");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("bloody", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, dc_set_nonexisting_key_in_existing_subsection_outside_range)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@1.key_string@2");
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);
}

TEST_F (DisirConfigSetGetKeyvalString,
        dc_set_nonexisting_key_in_nonexisting_subsection_outside_range)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@2.key_string@2");
    EXPECT_STATUS (DISIR_STATUS_CONFLICT, status);
    EXPECT_STREQ ("accessing non-existent index key_string@2, expected index 1",
                  dc_context_error (context_config));
}

TEST_F (DisirConfigSetGetKeyvalString,
        dc_set_nonexisting_key_in_nonexisting_subsection_exceeds_maximum)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@2.key_string@6");
    EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);
    EXPECT_STREQ ("accessing index key_string@6 exceeds maximum allowed instances of 3",
                  dc_context_error (context_config));
}

TEST_F (DisirConfigSetGetKeyvalString, dc_set_invalid_key_in_existing_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@1.bloody_key");
    EXPECT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    EXPECT_STREQ ("keyval bloody_key does not exist", dc_context_error (context_config));
}

TEST_F (DisirConfigSetGetKeyvalString, dc_set_invalid_key_in_nonexisting_subsection)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@2.bloody_key");
    EXPECT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    EXPECT_STREQ ("keyval bloody_key does not exist", dc_context_error (context_config));
}

TEST_F (DisirConfigSetGetKeyvalString,
        dc_set_existing_key_in_nonexisting_subsection_that_is_creatable)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_config_set_keyval_string (context_config, "bloody", "first@2.key_string");
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ (NULL, dc_context_error (context_config));
}

TEST_F (DisirConfigSetGetKeyvalString, disir_get_valid_key)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_get_keyval_string (config, &string_value, "first.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("string_value", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, disir_config_get_keyval_null_argument)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_get_keyval_string (NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (config, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (NULL, &string_value, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (config, &string_value, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (config, NULL, "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (NULL, &string_value, "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_get_keyval_string (NULL, NULL, "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (DisirConfigSetGetKeyvalString, disir_set_valid_key)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_set_keyval_string (config, "bloody hell", "first.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_config_get_keyval_string (config, &string_value, "first.key_string");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("bloody hell", string_value);
}

TEST_F (DisirConfigSetGetKeyvalString, disir_config_set_keyval_null_argument)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_set_keyval_string (NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (config, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (NULL, "random value", NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (config, "random value", NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (config, NULL, "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (NULL, "random value", "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_config_set_keyval_string (NULL, NULL, "random query");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}


// TODO: Add dc_config_get/set tests for interacting with existing, wrong type keys
// and stuff that breeches maximum entries
