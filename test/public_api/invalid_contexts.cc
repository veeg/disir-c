// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class InvalidContextsTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        mold = NULL;
        context_config = NULL;
        context_section = NULL;

        status = disir_mold_read (instance, "test", "basic_section", &mold);
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
    struct disir_context *context_keyval;
};

TEST_F (InvalidContextsTest, set_invalid_section_name)
{
    status = dc_set_name (context_section, "invalid_name", strlen ("invalid_name"));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_putcontext (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InvalidContextsTest, set_context_on_invalid_parent)
{
    status = dc_set_name (context_section, "invalid_name", strlen ("invalid_name"));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "invalid_name", strlen ("invalid_name"));
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_set_value_integer (context_keyval, 1);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_TRUE (context_keyval != NULL);
    ASSERT_TRUE (context_section != NULL);

    status = dc_putcontext (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_putcontext (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

