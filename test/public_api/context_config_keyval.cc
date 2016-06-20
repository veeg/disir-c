// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class ConfigKeyvalTest : public testing::DisirTestTestPlugin
{
protected:
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        mold = NULL;
        context_mold = NULL;
        context_config = NULL;
        context_keyval = NULL;

        status = disir_mold_input (instance, "test", "basic_keyval", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_config != NULL);

        status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_keyval != NULL);
    }

    void TearDown()
    {
        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_mold)
        {
            status = dc_putcontext (&context_mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_destroy (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_keyval)
        {
            status = dc_destroy (&context_keyval);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_mold *mold;
    struct disir_context *context_mold;
    struct disir_context *context_config;
    struct disir_context *context_keyval;
};

class ConfigKeyvalIntegerTest : public ConfigKeyvalTest
{
protected:
    void SetUp()
    {
        ConfigKeyvalTest::SetUp ();

        status = dc_set_name (context_keyval, "key_integer", strlen ("key_integer"));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }
};

class ConfigKeyvalStringTest : public ConfigKeyvalTest
{
protected:
    void SetUp()
    {
        ConfigKeyvalTest::SetUp ();

        status = dc_set_name (context_keyval, "key_string", strlen ("key_string"));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }
};


TEST_F (ConfigKeyvalTest, set_name_doesnt_exist_shall_fail)
{
    const char name[] = "this_name_doesnt_exist";

    status = dc_set_name (context_keyval, name, strlen (name));
    // XXX: Should be a better return status
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    // XXX Assert error message
}

TEST_F (ConfigKeyvalTest, set_name_that_exists_shall_succeed)
{
    status = dc_set_name (context_keyval, "key_string", strlen ("key_string"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}


TEST_F (ConfigKeyvalTest, set_integer_value_before_name_shall_fail)
{
    status = dc_set_value_integer (context_keyval, 72);
    ASSERT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    ASSERT_STREQ ("cannot set value on context without a MOLD",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalTest, set_string_value_before_name_shall_fail)
{
    status = dc_set_value_string (context_keyval, "mys", strlen ("mys"));
    ASSERT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    ASSERT_STREQ ("cannot set value on context without a MOLD",
                  dc_context_error (context_keyval));

}

TEST_F (ConfigKeyvalIntegerTest, set_value_integer_shall_succeed)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalIntegerTest, set_value_string_shall_fail)
{
    status = dc_set_value_string (context_keyval, "mys", strlen ("mys"));
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot set string value on context whose value type is INTEGER",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalStringTest, set_value_string_shall_succeed)
{
    status = dc_set_value_string (context_keyval, "hooray", strlen ("hooray"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalStringTest, set_value_integer_shall_fail)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot set integer value on context whose value type is STRING",
                  dc_context_error (context_keyval));
}
