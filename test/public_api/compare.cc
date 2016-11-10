
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class CompareTest : public testing::DisirTestTestPlugin
{
protected:
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();
        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (mold1)
        {
            disir_mold_finished (&mold1);
        }
        if (mold2)
        {
            disir_mold_finished (&mold2);
        }
        if (config1)
        {
            disir_config_finished (&config1);
        }
        if (config2)
        {
            disir_config_finished (&config2);
        }
        if (context_config1)
        {
            dc_putcontext (&context_config1);
        }
        if (context_config2)
        {
            dc_putcontext (&context_config2);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:

    void read_mold (const char *entry, struct disir_mold **mold)
    {
        status = disir_mold_read (instance, entry, mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void read_config1 (const char *entry)
    {
        status = disir_config_read (instance, entry, NULL, &config1);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_config1 = dc_config_getcontext (config1);
    }

    void read_config2 (const char *entry)
    {
        status = disir_config_read (instance, entry, NULL, &config2);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        context_config2 = dc_config_getcontext (config2);
    }

    enum disir_status status = DISIR_STATUS_OK;
    struct disir_mold *mold1 = NULL;
    struct disir_mold *mold2 = NULL;
    struct disir_config *config1 = NULL;
    struct disir_config *config2 = NULL;
    struct disir_context *context_config1 = NULL;
    struct disir_context *context_config2 = NULL;
};

TEST_F (CompareTest, config_basic_keyval)
{
    read_config1 ("basic_keyval");
    read_config2 ("basic_keyval");

    DisirLogCurrentTest ("Comparing basic_keyval");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_json_test_mold)
{
    read_config1 ("json_test_mold");
    read_config2 ("json_test_mold");

    DisirLogCurrentTest ("Comparing json_test_mold");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_keyval_numeric_types)
{
    read_config1 ("restriction_keyval_numeric_types");
    read_config2 ("restriction_keyval_numeric_types");

    DisirLogCurrentTest ("Comparing restriction_keyval_numeric_types");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_entries)
{
    read_config1 ("restriction_entries");
    read_config2 ("restriction_entries");

    DisirLogCurrentTest ("Comparing restriction_entries");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_config_parent_keyval_min_entry)
{
    read_config1 ("restriction_config_parent_keyval_min_entry");
    read_config2 ("restriction_config_parent_keyval_min_entry");

    DisirLogCurrentTest ("Comparing restriction_config_parent_keyval_min_entry");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_config_parent_keyval_max_entry)
{
    read_config1 ("restriction_config_parent_keyval_max_entry");
    read_config2 ("restriction_config_parent_keyval_max_entry");

    DisirLogCurrentTest ("Comparing restriction_config_parent_keyval_max_entry");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_config_parent_section_max_entry)
{
    read_config1 ("restriction_config_parent_section_max_entry");
    read_config2 ("restriction_config_parent_section_max_entry");

    DisirLogCurrentTest ("Comparing restriction_config_parent_section_max_entry");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_restriction_section_parent_keyval_max_entry)
{
    read_config1 ("restriction_section_parent_keyval_max_entry");
    read_config2 ("restriction_section_parent_keyval_max_entry");

    DisirLogCurrentTest ("Comparing restriction_section_parent_keyval_max_entry");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_basic_version_difference)
{
    read_config1 ("basic_version_difference");
    read_config2 ("basic_version_difference");

    DisirLogCurrentTest ("Comparing basic_version_differnce");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, complex_section)
{
    read_config1 ("complex_section");
    read_config2 ("complex_section");

    DisirLogCurrentTest ("Comparing complex_section");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (CompareTest, config_basic_keyval_with_basic_section)
{
    read_config1 ("basic_keyval");
    read_config2 ("basic_section");

    DisirLogCurrentTest ("Comparing basic_keyval with basic_section");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);
}

TEST_F (CompareTest, config_basic_keyval_with_complex_section)
{
    read_config1 ("basic_keyval");
    read_config2 ("complex_section");

    DisirLogCurrentTest ("Comparing basic_keyval with complex_section");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);
}

TEST_F (CompareTest, config_basic_section_with_complex_section)
{
    read_config1 ("basic_section");
    read_config2 ("complex_section");

    DisirLogCurrentTest ("Comparing basic_section with complex_section");
    status = dc_compare (context_config1, context_config2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);
}

