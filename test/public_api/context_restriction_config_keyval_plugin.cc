
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"

class ContextRestrictionConfigConstructingKeyvalPluginTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_mold_read (instance, "test", "restriction_keyval_numeric_types", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (mold)
        {
            disir_mold_finished (&mold);
        }
        if (context_config)
        {
            dc_destroy (&context_config);
        }
        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    const char *error;
    struct disir_mold *mold = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
};

class ContextRestrictionConfigFinalizedKeyvalPluginTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_read (instance, "test", "restriction_keyval_numeric_types",
                                     NULL, &config);
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
        if (context_integer)
        {
            dc_putcontext (&context_integer);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    const char *error;
    struct disir_config *config = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_integer = NULL;
};


TEST_F (ContextRestrictionConfigFinalizedKeyvalPluginTest, integer_restriction_set_valid)
{
    status = dc_find_element (context_config, "viterbi_encoders", 0, &context_integer);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_integer (context_integer, 8);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextRestrictionConfigFinalizedKeyvalPluginTest, integer_restriction_set_invalid)
{
    int64_t value;

    status = dc_find_element (context_config, "viterbi_encoders", 0, &context_integer);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // We are not allowed to set restriction violated value on finalized keyval
    status = dc_set_value_integer (context_integer, 12);
    ASSERT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);

    // Check that value did not change
    // default is 2
    status = dc_get_value_integer (context_integer, &value);
    ASSERT_EQ (2, value);

    // Context is still valid
    status = dc_context_valid (context_integer);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextRestrictionConfigFinalizedKeyvalPluginTest,
        finalizing_invalid_keyval_does_not_add_to_element_storage)
{
    struct disir_context *context;
    struct disir_collection *collection;
    int32_t before;
    int32_t after;

    status = dc_get_elements (context_config, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    before = dc_collection_size (collection);
    status = dc_collection_finished (&collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // We are not allowed to set restriction violated value on finalized keyval
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context, "invalid_name", strlen ("invalid_name"));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    status = dc_finalize (&context);
    ASSERT_STATUS (DISIR_STATUS_MOLD_MISSING, status);
    dc_putcontext (&context);

    status = dc_get_elements (context_config, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    after = dc_collection_size (collection);
    status = dc_collection_finished (&collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (before, after);
}

TEST_F (ContextRestrictionConfigConstructingKeyvalPluginTest, integer_restriction_set_invalid)
{
    int64_t value;

    status = dc_set_name (context_keyval, "viterbi_encoders", strlen ("viterbi_encoders"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // We are allowed to set the value, but it will become invalid
    status = dc_set_value_integer (context_keyval, 12);
    ASSERT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);

    // Assert context is invalid
    status = dc_context_valid (context_keyval);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Value is set to 12, even though it was invalid.
    status = dc_get_value_integer (context_keyval, &value);
    ASSERT_EQ (12, value);
}

TEST_F (ContextRestrictionConfigConstructingKeyvalPluginTest, float_restriction_set_invalid)
{
    double value;

    status = dc_set_name (context_keyval, "float_complex", strlen ("float_complex"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // We are allowed to set the value, but it will become invalid
    status = dc_set_value_float (context_keyval, 45.87);
    ASSERT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);

    // Assert context is invalid
    status = dc_context_valid (context_keyval);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Value is set to 12, even though it was invalid.
    status = dc_get_value_float (context_keyval, &value);
    ASSERT_EQ (45.87, value);
}

