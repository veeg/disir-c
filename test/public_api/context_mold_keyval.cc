
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class MoldKeyvalTest : public testing::DisirTestWrapper
{
protected:
    void SetUp()
    {
        DisirLogCurrentTestEnter ();

        context = NULL;
        context_mold = NULL;
        context_keyval = NULL;

        status = dc_mold_begin (&context_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_mold, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown()
    {
        if (context)
        {
            dc_destroy (&context);
        }
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
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
};


class MoldKeyvalStringTest : public MoldKeyvalTest
{
    void SetUp ()
    {
        MoldKeyvalTest::SetUp ();

        status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        const char rname[] = "test_keyval_name";
        status = dc_set_name (context_keyval, rname, strlen (rname));
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }
};


TEST_F (MoldKeyvalTest, context_type)
{
    ASSERT_EQ (dc_context_type (context_keyval), DISIR_CONTEXT_KEYVAL);
}

TEST_F (MoldKeyvalTest, context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context_keyval), "KEYVAL");
}

TEST_F (MoldKeyvalTest, set_name)
{
    const char rname[] = "test_keyval_name";

    status = dc_set_name (context_keyval, rname, strlen (rname));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalTest, get_name)
{
    const char rname[] = "test_keyval_name";
    const char *qname;
    int32_t size;

    // Setup
    status = dc_set_name (context_keyval, rname, strlen (rname));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_name (context_keyval, &qname, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_name (context_keyval, &qname, &size);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_STREQ (rname, qname);
    ASSERT_EQ (strlen (rname), size);
}

TEST_F (MoldKeyvalTest, add_documentation)
{
    const char doc[] = "test doc string";

    status = dc_add_documentation (context_keyval, doc, strlen (doc));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalTest, get_documentation)
{
    const char doc[] = "test doc string";
    const char *qdoc;
    int32_t size;

    // setup
    status = dc_add_documentation (context_keyval, doc, strlen (doc));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_get_documentation (context_keyval, NULL, &qdoc, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ (doc, qdoc);

    status = dc_get_documentation (context_keyval, NULL, &qdoc, &size);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (strlen (doc), size);
}

TEST_F (MoldKeyvalTest, set_type_string)
{
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_STRING);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalTest, set_type_int)
{
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_INTEGER);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalTest, set_type_float)
{
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_FLOAT);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalTest, set_type_boolean)
{
    status = dc_set_value_type (context_keyval, DISIR_VALUE_TYPE_BOOLEAN);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalStringTest, add_string_default_shall_succeed)
{
    const char testval[] = "test string value";

    status = dc_add_default_string (context_keyval, testval, strlen (testval), NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MoldKeyvalStringTest, add_int_default_shall_fail)
{
    status = dc_add_default_integer (context_keyval, 42, NULL);
    // XXX TODO: This should probably return a more descriptive error?
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("cannot set integer value on context whose value type is STRING",
                  dc_context_error (context_keyval));
}


