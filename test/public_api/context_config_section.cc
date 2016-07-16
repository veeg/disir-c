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

        status = disir_mold_input (instance, "test", "basic_section", &mold);
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

