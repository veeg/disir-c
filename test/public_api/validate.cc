
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

        status = disir_config_read (instance, "test", "basic_keyval", NULL, &bkeyval_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (bkeyval_config != NULL);

        status = disir_mold_read (instance, "test", "basic_keyval", &bkeyval_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (bkeyval_mold != NULL);

        status = disir_mold_read (instance, "test", "basic_section", &bsection_mold);
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

        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        if (config)
        {
            status = disir_config_finished (&config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        if (context_config)
        {
            status = dc_putcontext (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:

    void setup_testmold (const char *name)
    {
        status = disir_mold_read (instance, "test", name, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_mold   *mold = NULL;
    struct disir_mold   *bkeyval_mold = NULL;
    struct disir_mold   *bsection_mold = NULL;
    struct disir_config *bkeyval_config = NULL;
    struct disir_config *config = NULL;
    struct disir_collection *collection = NULL;
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

    // create the 4 required keys so the config is valid
    // string
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "key_string", strlen ("key_string"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    // int
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "key_integer", strlen ("key_integer"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    // float
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "key_float", strlen ("key_float"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    // bool
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "key_boolean", strlen ("key_boolean"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);


    // Setup keyval that is invalid
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, name, strlen (name));
    EXPECT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
    ASSERT_TRUE (context_keyval != NULL);

    // Finalize config - it is invalid because it contains an invalid child.
    status = dc_config_finalize (&context_config, &config);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

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

     // Assert config is valid
    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // cleanup
    disir_config_finished (&config);
}

TEST_F (ValidateTest, generate_config_basic_section)
{
    struct disir_config *config;

    status = disir_generate_config_from_mold (bsection_mold, NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert config is valid
    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // cleanup
    disir_config_finished (&config);
}

// Assert that disir_generate_config_from_mold respects min entries restriction
TEST_F (ValidateTest, generate_config_restriction_min_entries)
{
    struct disir_config *config;
    struct disir_context *context;
    int size;

    setup_testmold ("restriction_config_parent_keyval_min_entry");

    // Version 2.0.0 - 4 min entries
    status = disir_generate_config_from_mold (mold, NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert that size of entries generated in config is 4.
    context = dc_config_getcontext (config);
    ASSERT_TRUE (context != NULL);
    status = dc_get_elements (context , &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    size = dc_collection_size (collection);
    ASSERT_EQ (4, size);

    // Assert config is valid
    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // cleanup
    status = dc_putcontext (&context);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = disir_config_finished (&config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}


TEST_F (ValidateTest, generate_config_version_is_sat_correctly)
{
    struct disir_config *config;
    struct disir_version version;
    struct disir_version queried;
    int diff;

    setup_testmold ("basic_version_difference");
    version.sv_major = 3;
    version.sv_minor = 0;
    queried.sv_major = 0;
    queried.sv_minor = 0;
    // TODO: Create a mold that contains simple keyvals that only differ in introduced version.

    // This should generate a config with version 3.0.0
    status = disir_generate_config_from_mold (mold, NULL, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_get_version (config, &queried);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    diff = dc_version_compare (&version, &queried);
    EXPECT_EQ (0, diff);
    // cleanup
    disir_config_finished (&config);

    // This should generate a config with version 2.0.0
    version.sv_major = 2;
    status = disir_generate_config_from_mold (mold, &version, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_get_version (config, &queried);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    diff = dc_version_compare (&version, &queried);
    EXPECT_EQ (0, diff);
    // cleanup
    disir_config_finished (&config);

    // This should generate a config with version 2.5.0
    version.sv_major = 2;
    version.sv_minor = 5;
    status = disir_generate_config_from_mold (mold, &version, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_get_version (config, &queried);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    diff = dc_version_compare (&version, &queried);
    EXPECT_EQ (0, diff);
     // cleanup
    disir_config_finished (&config);
}

