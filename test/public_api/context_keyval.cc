
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


// Test mold API with empty mold.
class ContextKeyvalTest : public testing::DisirTestWrapper
{
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


TEST_F (ContextKeyvalTest, context_type)
{
    ASSERT_EQ (dc_context_type (context_keyval), DISIR_CONTEXT_KEYVAL);
}

TEST_F (ContextKeyvalTest, context_type_string)
{
    ASSERT_STREQ (dc_context_type_string (context_keyval), "KEYVAL");
}

TEST_F (ContextKeyvalTest, set_name)
{
    const char rname[] = "test_keyval_name";

    status = dc_set_name (context_keyval, rname, strlen (rname));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextKeyvalTest, get_name)
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

