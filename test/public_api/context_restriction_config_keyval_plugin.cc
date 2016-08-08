
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class ContextRestrictionConfigKeyvalPluginTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_input (instance, "test", "restriction_keyval_numeric_types",
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


TEST_F (ContextRestrictionConfigKeyvalPluginTest, integer_restriction_set_valid)
{
    status = dc_find_element (context_config, "viterbi_encoders", 0, &context_integer);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_integer (context_integer, 8);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ContextRestrictionConfigKeyvalPluginTest, integer_restriction_set_invalid)
{
    status = dc_find_element (context_config, "viterbi_encoders", 0, &context_integer);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // TODO: Implement and set correct erroneous return code.
    status = dc_set_value_integer (context_integer, 12);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

