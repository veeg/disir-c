// PUBLIC API
#include <disir/disir.h>
#include <disir/context.h>
#include <disir/config.h>

// TEST API
#include "test_helper.h"


//
// This class tests the public API functions:
//  dc_restriction_entries_minimum
//  dc_restriction_entries_maximum
//
class DcRestrictionEntriesMinimumMaximum : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_config_read (instance, "test", "restriction_entries",
                                    NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = disir_mold_read (instance, "test", "restriction_entries", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        context_config = dc_config_getcontext (config);
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
        if (mold)
        {
            disir_mold_finished (&mold);
        }
        if (context_mold)
        {
            dc_putcontext (&context_mold);
        }

        if (context_query)
        {
            dc_putcontext (&context_query);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    int output = -2;
    struct disir_context *context_config = NULL;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_query = NULL;
    struct disir_config *config = NULL;
    struct disir_mold *mold = NULL;
};

TEST_F (DcRestrictionEntriesMinimumMaximum, minimum_unsupported_context)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_restriction_entries_minimum (context_config, &output);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    status = dc_restriction_entries_minimum (context_mold, &output);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (DcRestrictionEntriesMinimumMaximum, maximum_unsupported_context)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_restriction_entries_maximum (context_config, &output);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    status = dc_restriction_entries_maximum (context_mold, &output);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (DcRestrictionEntriesMinimumMaximum, minimum_no_restriction_set)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_find_element (context_mold, "keyval_default", 0, &context_query);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_entries_minimum (context_query, &output);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (0, output);
}

TEST_F (DcRestrictionEntriesMinimumMaximum, maximum_no_restriction_set)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_find_element (context_mold, "keyval_default", 0, &context_query);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_entries_maximum (context_query, &output);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (1, output);
}

TEST_F (DcRestrictionEntriesMinimumMaximum, minimum_restriction_set)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_find_element (context_mold, "keyval_complex", 0, &context_query);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_entries_minimum (context_query, &output);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (3, output);
}

TEST_F (DcRestrictionEntriesMinimumMaximum, maximum_restriction_set)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_find_element (context_mold, "keyval_complex", 0, &context_query);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_entries_maximum (context_query, &output);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (4, output);
}
