#include <gtest/gtest.h>
#include <map>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "value.h"
}

#include "test_helper.h"


// Global recurring sample string
const char sample[] = "Snickers and Mars bars";

class DisirValueGenericTest: public testing::DisirTestWrapper
{
protected:
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        memset (&value, 0, sizeof (struct disir_value));
        value.dv_type = type;
    }

    void TearDown()
    {
        if (value.dv_type == DISIR_VALUE_TYPE_STRING)
        {
            if (value.dv_string)
            {
                free (value.dv_string);
            }
        }
        DisirLogCurrentTestExit ();
    }

public:
    enum disir_status status    = DISIR_STATUS_OK;;
    enum disir_value_type type  = DISIR_VALUE_TYPE_UNKNOWN;
    struct disir_value value;
};

class DisirValueStringTest : public DisirValueGenericTest
{
    void SetUp()
    {
        type = DISIR_VALUE_TYPE_STRING;
        DisirValueGenericTest::SetUp();
    }
};

class DisirValueIntegerTest : public DisirValueGenericTest
{
    void SetUp()
    {
        type = DISIR_VALUE_TYPE_INTEGER;
        DisirValueGenericTest::SetUp();
    }
};

class DisirValueFloatTest : public DisirValueGenericTest
{
    void SetUp()
    {
        type = DISIR_VALUE_TYPE_FLOAT;
        DisirValueGenericTest::SetUp();
    }
};

class DisirValueEnumTest : public DisirValueGenericTest
{
    void SetUp()
    {
        type = DISIR_VALUE_TYPE_ENUM;
        DisirValueGenericTest::SetUp();
    }
};

class DisirValueBooleanTest : public DisirValueGenericTest
{
    void SetUp()
    {
        type = DISIR_VALUE_TYPE_BOOLEAN;
        DisirValueGenericTest::SetUp();
    }
};



//
// DISIR VALUE GENERIC
//

TEST_F (DisirValueGenericTest, sanify_invalid)
{
    ASSERT_EQ (dx_value_type_sanify ((enum disir_value_type) 0), DISIR_VALUE_TYPE_UNKNOWN);
    ASSERT_EQ (dx_value_type_sanify ((enum disir_value_type) -1), DISIR_VALUE_TYPE_UNKNOWN);
    ASSERT_EQ (dx_value_type_sanify ((enum disir_value_type) 659843), DISIR_VALUE_TYPE_UNKNOWN);
}

//
// DISIR VALUE STRING
//

TEST_F (DisirValueStringTest, sanify)
{
    ASSERT_EQ (dx_value_type_sanify (type), DISIR_VALUE_TYPE_STRING);
}

TEST_F (DisirValueStringTest, string)
{
    ASSERT_STREQ (dx_value_type_string (type), "STRING");
}

TEST_F (DisirValueStringTest, set_string_invalid_argument_shall_fail)
{
    status = dx_value_set_string (NULL, NULL, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = dx_value_set_string (NULL, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (DisirValueStringTest, set_string_invalid_value_type_shall_fail)
{
    value.dv_type = DISIR_VALUE_TYPE_UNKNOWN;

    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}


TEST_F (DisirValueStringTest, set_string_valid_argument_shall_succeed)
{
    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ (sample, value.dv_string);
}

TEST_F (DisirValueStringTest, get_string_invalid_argument_shall_fail)
{
    const char *output;

    status = dx_value_get_string (NULL, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = dx_value_get_string (&value, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = dx_value_get_string (NULL, &output, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (DisirValueStringTest, get_string_populated_shall_succeed)
{
    const char *output;
    int32_t output_size;

    // Setup
    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_STREQ (sample, value.dv_string);

    status = dx_value_get_string (&value, &output, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_get_string (&value, &output, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    EXPECT_STREQ (sample, output);
    EXPECT_EQ (strlen (sample), output_size);
}


TEST_F (DisirValueStringTest, get_string_empty_shall_succeed)
{
    const char *output;
    int32_t output_size;

    status = dx_value_get_string (&value, &output, NULL);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_get_string (&value, &output, &output_size);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    EXPECT_EQ (NULL, output);
    EXPECT_EQ (0, output_size);
}

TEST_F (DisirValueStringTest, compare_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_STRING;

    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_string (&second, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_EQ (0, res);
}

TEST_F (DisirValueStringTest, compare_not_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_STRING;

    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_string (&second, "ABC", strlen ("ABC"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_GT (res, 0);
}

//
// DISIR VALUE INTEGER
//

TEST_F (DisirValueIntegerTest, sanify)
{
    ASSERT_EQ (dx_value_type_sanify (type), DISIR_VALUE_TYPE_INTEGER);
}

TEST_F (DisirValueIntegerTest, string)
{
    ASSERT_STREQ (dx_value_type_string (type), "INTEGER");
}

TEST_F (DisirValueIntegerTest, compare_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_INTEGER;

    status = dx_value_set_integer (&value, 5);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_integer (&second, 5);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_EQ (0, res);
}

TEST_F (DisirValueIntegerTest, compare_not_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_INTEGER;

    status = dx_value_set_integer (&value, 5);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_integer (&second, 2);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_GT (res, 0);
}

//
// DISIR VALUE FLOAT
//

TEST_F (DisirValueFloatTest, sanify)
{
    ASSERT_EQ (dx_value_type_sanify (type), DISIR_VALUE_TYPE_FLOAT);
}

TEST_F (DisirValueFloatTest, string)
{
    ASSERT_STREQ (dx_value_type_string (type), "FLOAT");
}


TEST_F (DisirValueFloatTest, compare_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_FLOAT;

    status = dx_value_set_float (&value, 14.3);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_float (&second, 14.3);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_EQ (0, res);
}

TEST_F (DisirValueFloatTest, compare_not_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_FLOAT;

    status = dx_value_set_float (&value, 10.5);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_float (&second, 13.4);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_LT (res, 0);
}

//
// DISIR VALUE ENUM
//

TEST_F (DisirValueEnumTest, sanify)
{
    ASSERT_EQ (dx_value_type_sanify (type), DISIR_VALUE_TYPE_ENUM);
}

TEST_F (DisirValueEnumTest, string)
{
    ASSERT_STREQ (dx_value_type_string (type), "ENUM");
}

TEST_F (DisirValueEnumTest, compare_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_ENUM;

    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_string (&second, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_EQ (0, res);
}

TEST_F (DisirValueEnumTest, compare_not_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_ENUM;

    status = dx_value_set_string (&value, sample, strlen (sample));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_string (&second, "ABC", strlen ("ABC"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_GT (res, 0);
}

//
// DISIR VALUE BOOLEAN
//

TEST_F (DisirValueBooleanTest, sanify)
{
    ASSERT_EQ (dx_value_type_sanify (type), DISIR_VALUE_TYPE_BOOLEAN);
}

TEST_F (DisirValueBooleanTest, string)
{
    ASSERT_STREQ (dx_value_type_string (type), "BOOLEAN");
}

TEST_F (DisirValueBooleanTest, compare_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_BOOLEAN;

    status = dx_value_set_boolean (&value, 1);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_boolean (&second, 1);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_EQ (0, res);
}

TEST_F (DisirValueBooleanTest, compare_not_equal)
{
    int res;
    struct disir_value second;
    memset (&second, 0, sizeof (struct disir_value));
    second.dv_type = DISIR_VALUE_TYPE_BOOLEAN;

    status = dx_value_set_boolean (&value, 0);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dx_value_set_boolean (&second, 1);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    res = dx_value_compare (&value, &second);
    ASSERT_LT (res, 0);
}

