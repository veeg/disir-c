#include <climits>

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

class ConfigKeyvalFloatTest : public ConfigKeyvalTest
{
protected:
    void SetUp()
    {
        ConfigKeyvalTest::SetUp ();

        status = dc_set_name (context_keyval, "key_float", strlen ("key_float"));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }
};

class ConfigKeyvalBooleanTest : public ConfigKeyvalTest
{
protected:
    void SetUp()
    {
        ConfigKeyvalTest::SetUp ();

        status = dc_set_name (context_keyval, "key_boolean", strlen ("key_boolean"));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }
};

TEST_F (ConfigKeyvalTest, set_name_doesnt_exist_shall_fail)
{
    const char name[] = "this_name_doesnt_exist";

    status = dc_set_name (context_keyval, name, strlen (name));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

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

TEST_F (ConfigKeyvalTest, set_float_value_before_name_shall_fail)
{
    status = dc_set_value_float (context_keyval, 3.12);
    ASSERT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    ASSERT_STREQ ("cannot set value on context without a MOLD",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalTest, set_section_name_on_keyval_shall_fail)
{
    struct disir_context *context_mold;
    struct disir_context *context_section;
    // Add section_name to mold
    context_mold = dc_mold_getcontext (mold);
    ASSERT_TRUE (context_mold != NULL);
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_section, "doc", strlen ("doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_putcontext (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Test - setting a matching section name is not valid.
    status = dc_set_name (context_keyval, "section_name", strlen ("section_name"));
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
    // XXX: Use DISIR_STATUS_WRONG_TYPE?
}

TEST_F (ConfigKeyvalIntegerTest, set_value_integer_shall_succeed)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalIntegerTest, get_value_integer_shall_succeed)
{
    int64_t value = LLONG_MAX;
    status = dc_get_value_integer (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (0, value);
}

TEST_F (ConfigKeyvalIntegerTest, set_value_string_shall_fail)
{
    status = dc_set_value_string (context_keyval, "mys", strlen ("mys"));
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type STRING, expecting INTEGER",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalIntegerTest, get_value_string_shall_fail)
{
    const char *output;
    int32_t size;

    status = dc_get_value_string (context_keyval, &output, &size);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get string value on context whose value type is INTEGER",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalStringTest, set_value_string_shall_succeed)
{
    status = dc_set_value_string (context_keyval, "hooray", strlen ("hooray"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalStringTest, get_value_string_shall_succeed)
{
    const char *output;
    int32_t size;

    status = dc_get_value_string (context_keyval, &output, &size);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalStringTest, set_value_integer_shall_fail)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type INTEGER, expecting STRING",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalStringTest, get_value_integer_shall_fail)
{
    int64_t value = LLONG_MAX;
    status = dc_get_value_integer (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}

TEST_F (ConfigKeyvalFloatTest, set_value_float_shall_succeed)
{
    status = dc_set_value_float (context_keyval, 42.123);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalFloatTest, get_value_float_shall_succeed)
{
    double value = LONG_MAX;

    status = dc_get_value_float (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (0.0, value);
}

TEST_F (ConfigKeyvalFloatTest, set_value_string_shall_fail)
{
    status = dc_set_value_string (context_keyval, "hooray", strlen ("hooray"));
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type STRING, expecting FLOAT",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalFloatTest, get_value_string_shall_fail)
{
    const char *output;
    int32_t size;

    status = dc_get_value_string (context_keyval, &output, &size);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get string value on context whose value type is FLOAT",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalFloatTest, set_value_integer_shall_fail)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type INTEGER, expecting FLOAT",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalFloatTest, get_value_integer_shall_fail)
{
    int64_t value = LLONG_MAX;
    status = dc_get_value_integer (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get integer value on context whose value type is FLOAT",
                  dc_context_error (context_keyval));
}

// XX
TEST_F (ConfigKeyvalBooleanTest, set_value_boolean_shall_succeed)
{
    status = dc_set_value_boolean (context_keyval, 1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ConfigKeyvalBooleanTest, get_value_boolean_shall_succeed)
{
    uint8_t value = 8;

    status = dc_get_value_boolean (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (0, value);
}

TEST_F (ConfigKeyvalBooleanTest, set_value_string_shall_fail)
{
    status = dc_set_value_string (context_keyval, "hooray", strlen ("hooray"));
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type STRING, expecting BOOLEAN",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalBooleanTest, get_value_string_shall_fail)
{
    const char *output;
    int32_t size;

    status = dc_get_value_string (context_keyval, &output, &size);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get string value on context whose value type is BOOLEAN",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalBooleanTest, set_value_integer_shall_fail)
{
    status = dc_set_value_integer (context_keyval, 74);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type INTEGER, expecting BOOLEAN",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalBooleanTest, get_value_integer_shall_fail)
{
    int64_t value = LLONG_MAX;
    status = dc_get_value_integer (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get integer value on context whose value type is BOOLEAN",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalBooleanTest, set_value_float_shall_fail)
{
    status = dc_set_value_float (context_keyval, 42.123);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_STREQ ("Assigned value type FLOAT, expecting BOOLEAN",
                  dc_context_error (context_keyval));
}

TEST_F (ConfigKeyvalBooleanTest, get_value_float_shall_fail)
{
    double value = LONG_MAX;

    status = dc_get_value_float (context_keyval, &value);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("cannot get float value on context whose value type is BOOLEAN",
                  dc_context_error (context_keyval));
}
