
#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class ValidateTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = DISIR_STATUS_OK;
        collection = NULL;
        bkeyval_mold = NULL;
        bsection_mold = NULL;
        bkeyval_config = NULL;
        config = NULL;
        context_config = NULL;
        context_keyval = NULL;
        context = NULL;

        status = disir_config_input (instance, "test", "basic_keyval", NULL, &bkeyval_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (bkeyval_config != NULL);

        status = disir_mold_input (instance, "test", "basic_keyval", &bkeyval_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (bkeyval_mold != NULL);

        status = disir_mold_input (instance, "test", "basic_section", &bsection_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (bkeyval_mold != NULL);


        status = dc_config_begin (bkeyval_mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (bkeyval_mold)
        {
            status = disir_mold_finished (&bkeyval_mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (bsection_mold)
        {
            status = disir_mold_finished (&bsection_mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (bkeyval_config)
        {
            status = disir_config_finished (&bkeyval_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (collection)
        {
            status = dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_config;
    struct disir_context *context_keyval;
    struct disir_mold   *bkeyval_mold;
    struct disir_mold   *bsection_mold = NULL;
    struct disir_config *bkeyval_config;
    struct disir_config *config;
    struct disir_collection *collection;
};

TEST_F (ValidateTest, disir_config_valid_test_basic_keyval_shall_succeed)
{
    status = disir_config_valid (bkeyval_config, &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    EXPECT_EQ (NULL, collection);
}

TEST_F (ValidateTest, config_keyval_set_invalid_name)
{
    const char name[] = "invalid_name";

    // Setup keyval that is invalid
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, name, strlen (name));
    EXPECT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (context_keyval != NULL);

    // Finalize config
    status = dc_config_finalize (&context_config, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert that config is not valid.
    status = disir_config_valid (config, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (collection != NULL);
    EXPECT_EQ (1, dc_collection_size (collection));

    dc_collection_next (collection, &context);
    ASSERT_EQ (context_keyval, context);

    ASSERT_STREQ ("KEYVAL missing mold equivalent entry for name 'invalid_name'.",
                  dc_context_error (context));

    dc_putcontext (&context_keyval);
    dc_putcontext (&context);
}

TEST_F (ValidateTest, generate_config_basic_keyval)
{
    struct disir_config *config;

    status = disir_generate_config_from_mold (bkeyval_mold, NULL, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // cleanup
    disir_config_finished (&config);
}

TEST_F (ValidateTest, generate_config_basic_section)
{
    struct disir_config *config;

    status = disir_generate_config_from_mold (bsection_mold, NULL, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // cleanup
    disir_config_finished (&config);
}
