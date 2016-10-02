#include <gtest/gtest.h>

#include "test_helper.h"

#include <disir/disir.h>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "context_private.h"
}

class MoldEquivTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = disir_mold_read (instance, "basic_section", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);
    }

    void TearDown()
    {
        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_destroy (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }
public:
    enum disir_status status;
    struct disir_mold *mold = NULL;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_section = NULL;
};

TEST_F (MoldEquivTest, test_get_correct_mold_equiv_type)
{
    enum disir_context_type type;
    status = dc_config_begin (mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
   
    status = dc_begin (context_config,
            DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dx_get_mold_equiv_type (context_section, "k1", &type);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (DISIR_CONTEXT_KEYVAL, type);
}

TEST_F (MoldEquivTest, mold_equiv_should_not_exist)
{
    enum disir_context_type type;
    status = dc_config_begin (mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config,
            DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dx_get_mold_equiv_type (context_section, "wrong", &type);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
}

TEST_F (MoldEquivTest, section_mold_equiv_should_exist)
{
    enum disir_context_type type;
    status = dc_config_begin (mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dx_get_mold_equiv_type (context_config, "section_name", &type);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (DISIR_CONTEXT_SECTION, type);
}

TEST_F (MoldEquivTest, fail_on_invalid_input)
{
    status = dx_get_mold_equiv_type (NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (MoldEquivTest, fail_on_wrong_context)
{
    struct disir_context *context_keyval;
    enum disir_context_type type;

    status = dc_config_begin (mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dx_get_mold_equiv_type (context_keyval, "fakename", &type);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);

}

TEST_F (MoldEquivTest, fail_on_wrong_root_context)
{
    struct disir_context *mold_context;
    enum disir_context_type type;

    mold_context = dc_mold_getcontext (mold);

    status = dx_get_mold_equiv_type (mold_context, "fakename", &type);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

