// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class ConfigSectionTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        mold = NULL;
        context_config = NULL;
        context_section = NULL;

        status = disir_mold_read (instance, "basic_section", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);

        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_config != NULL);

        status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (context_section != NULL);

        DisirLogCurrentTest ("SetUp completed - TestBody:");
    }

    void TearDown()
    {
        DisirLogCurrentTest ("TestBody completed - TearDown:");
        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        if (context_config)
        {
            status = dc_destroy (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_mold *mold;
    struct disir_context *context_config;
    struct disir_context *context_section;
};


TEST_F (ConfigSectionTest, set_name_doesnt_exist_shall_fail)
{
    const char name[] = "this_name_doesnt_exist";

    status = dc_set_name (context_section, name, strlen (name));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    // XXX Assert error message

}

TEST_F (ConfigSectionTest, add_introduced_shall_fail)
{
    struct semantic_version input;

    input.sv_major = 1;
    input.sv_minor = 1;
    input.sv_patch = 2;

    status = dc_add_introduced (context_section, &input);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    ASSERT_STREQ ("Cannot add introduced to SECTION whose top-level is CONFIG.",
                  dc_context_error (context_section));
}

TEST_F (ConfigSectionTest, get_introduced_shall_fail)
{
    struct semantic_version semver;

    status = dc_get_introduced (context_section, &semver);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

    ASSERT_STREQ ("Cannot get introduced from SECTION whose top-level is CONFIG.",
                  dc_context_error (context_section));
}


TEST_F (ConfigSectionTest, set_keyval_name_on_section_shall_fail)
{
    struct disir_context *context_mold;
    // Add keyval_name to mold
    context_mold = dc_mold_getcontext (mold);
    ASSERT_TRUE (context_mold != NULL);
    status = dc_add_keyval_integer (context_mold, "keyval_name", 1, "doc", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_putcontext (&context_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Test - setting a matching section name is not valid.
    status = dc_set_name (context_section, "keyval_name", strlen ("keyval_name"));
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
    // XXX: Use DISIR_STATUS_WRONG_TYPE?
}

