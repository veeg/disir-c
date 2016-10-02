

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"

class ContextRestrictionConfigConstructingEntriesTestPluginTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

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
        if (context_section)
        {
            dc_destroy (&context_section);
        }
        if (config)
        {
            disir_config_finished (&config);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    void setup_testmold (const char *entry)
    {
        status = disir_mold_read (instance, entry, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

public:
    enum disir_status status;
    const char *error;
    struct disir_config *config= NULL;
    struct disir_mold *mold = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_section = NULL;
};


class ContextRestrictionConfigFinalizedEntriesTestPluginTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

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
            dc_putcontext (&context_config);
        }
        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }
        if (context_section)
        {
            dc_destroy (&context_section);
        }
        if (config)
        {
            disir_config_finished (&config);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    void setup_testconfig (const char *entry, struct semantic_version *semver)
    {
        status = disir_mold_read (instance, entry, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        if (semver)
        {
            status = dc_set_version (context_config, semver);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }
        status = dc_config_finalize (&context_config, &config);
        // This might be invalid
    }

public:
    enum disir_status status;
    const char *error;
    struct disir_config *config= NULL;
    struct disir_mold *mold = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_section = NULL;
};


TEST_F (ContextRestrictionConfigConstructingEntriesTestPluginTest,
        finalizing_config_without_required_elements_yields_invalid_config)
{
    setup_testmold ("restriction_config_parent_keyval_min_entry");

    // The element in this mold are required (non-zero entries)
    // Finalizing the config should yield DISIR_STATUS_CONTEXT_INVALID
    status = dc_config_finalize (&context_config, &config);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Config object should still be yielded, but invalid
    ASSERT_EQ (NULL, context_config);
    ASSERT_TRUE (config != NULL);

    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (ContextRestrictionConfigConstructingEntriesTestPluginTest,
        finalizing_config_with_max_entries_violated_yields_invalid_config)
{
    struct semantic_version semver;
    setup_testmold ("restriction_config_parent_keyval_max_entry");

    // Set config version to 1.0.0 - this sets max at 2
    semver.sv_major = 1;
    semver.sv_minor = 0;
    semver.sv_patch = 0;
    status = dc_set_version (context_config, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    // This instance will violate the config (since its the third of maximum 2 allowed)
    // This context however is not invalid
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // finalizing config yields invalid context
    status = dc_config_finalize (&context_config, &config);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Config object should still be yielded, but invalid
    ASSERT_EQ (NULL, context_config);
    ASSERT_TRUE (config != NULL);

    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (ContextRestrictionConfigConstructingEntriesTestPluginTest,
        finalizing_section_with_max_entries_violated_yields_invalid_config)
{
    struct semantic_version semver;
    setup_testmold ("restriction_section_parent_keyval_max_entry");

    // Set config version to 1.0.0
    semver.sv_major = 1;
    semver.sv_minor = 0;
    semver.sv_patch = 0;
    status = dc_set_version (context_config, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // allocate section object
    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section", strlen ("section"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add one instance
    // This instance will violate the section (since its the third of maximum 2 allowed)
    // This context however is not invalid
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);


    // finalizing section yields invalid context
    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // finalizing config yields invalid context
    status = dc_config_finalize (&context_config, &config);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    // Config object should still be yielded, but invalid
    ASSERT_EQ (NULL, context_config);
    ASSERT_TRUE (config != NULL);

    status = disir_config_valid (config, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

// Adding a keyval to a finalized config parent that violates max entries
// is not allowed.
// dc_set_name shall fail with DISIR_STATUS_RESTRICTION_VIOLATED
TEST_F (ContextRestrictionConfigFinalizedEntriesTestPluginTest,
        adding_keyval_to_config_parent_violating_max_restriction)
{
    // version 1.0.0
    setup_testconfig ("restriction_config_parent_keyval_max_entry", NULL);

    // version 1.0.0 has max 2 entries
    context_config = dc_config_getcontext (config);
    for (int i = 1; i <= 3; i++)
    {
        status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
        EXPECT_STATUS (DISIR_STATUS_OK, status);

        status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
        if (i != 3)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        else
        {
            EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);
        }

        status = dc_finalize (&context_keyval);
        if (i != 3)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
            EXPECT_EQ (NULL, context_keyval);
        }
        else
        {
            // no name
            EXPECT_STATUS (DISIR_STATUS_CONTEXT_IN_WRONG_STATE, status);
            dc_putcontext (&context_keyval);
        }
    }

    dc_putcontext (&context_config);
}

// Adding a keyval to a finalized config parent that violates max entries
// is not allowed.
// dc_set_name shall fail with DISIR_STATUS_RESTRICTION_VIOLATED
TEST_F (ContextRestrictionConfigFinalizedEntriesTestPluginTest,
        race_finalizing_keyval_to_config_parent_violating_max_restriction)
{
    struct disir_context *context_keyval2;
    // version 1.0.0
    setup_testconfig ("restriction_config_parent_keyval_max_entry", NULL);

    // version 1.0.0 has max 2 entries
    context_config = dc_config_getcontext (config);

    // Add one - this is fine
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status)

    // Begin second - this is fine -
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval2);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval2, "keyval", strlen ("keyval"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);


    // Begin third - this is fine
    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Finalize third - this is fine (second added to parent)
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Finalize second - this is not fine (adding third to parent, violating restriction)
    status = dc_finalize (&context_keyval2);
    EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);
    EXPECT_TRUE (context_keyval2 != NULL);

    // cleanup
    status = dc_destroy (&context_keyval2);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    dc_putcontext (&context_config);
}

TEST_F (ContextRestrictionConfigFinalizedEntriesTestPluginTest,
        adding_section_to_config_parent_violating_max_restriction)
{
    // version 1.0.0
    setup_testconfig ("restriction_config_parent_section_max_entry", NULL);

    // version 1.0.0 has max 4 entries
    context_config = dc_config_getcontext (config);
    for (int i = 1; i <= 5; i++)
    {
        status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
        EXPECT_STATUS (DISIR_STATUS_OK, status);

        status = dc_set_name (context_section, "section", strlen ("section"));
        if (i != 5)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        else
        {
            EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);
        }

        status = dc_finalize (&context_section);
        if (i != 5)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
            EXPECT_EQ (NULL, context_section);
        }
        else
        {
            // no name
            EXPECT_STATUS (DISIR_STATUS_CONTEXT_IN_WRONG_STATE, status);
            dc_putcontext (&context_section);
        }
    }

    dc_putcontext (&context_config);
}

TEST_F (ContextRestrictionConfigFinalizedEntriesTestPluginTest,
        race_finalizing_section_to_config_parent_violating_max_restriction)
{
    struct disir_context *context_section2;
    // version 1.0.0
    setup_testconfig ("restriction_config_parent_section_max_entry", NULL);

    // version 1.0.0 has max 4 entries
    context_config = dc_config_getcontext (config);

    // add three entires - this is fine.
    for (int i = 0; i < 3; i++)
    {
        status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        status = dc_set_name (context_section, "section", strlen ("section"));
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        status = dc_finalize (&context_section);
        EXPECT_STATUS (DISIR_STATUS_OK, status)
    }


    // Begin forth - dont finalize.
    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section", strlen ("section"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    // Begin fifth - finalize - this is fine.
    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section2);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section2, "section", strlen ("section"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section2);
    EXPECT_STATUS (DISIR_STATUS_OK, status)

    // Finalize forth - this is not fine (4 already exist)
    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);

    // cleanup
    status = dc_destroy (&context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    dc_putcontext (&context_config);
}

TEST_F (ContextRestrictionConfigFinalizedEntriesTestPluginTest,
        adding_keyval_to_section_parent_violating_max_restriction)
{
    // version 1.0.0
    setup_testconfig ("restriction_section_parent_keyval_max_entry", NULL);
    context_config = dc_config_getcontext (config);

    // Construct and finalize the section element.
    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section", strlen ("section"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_find_element (context_config, "section", 0, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // version 1.0.0 has max 2 entries
    for (int i = 1; i <= 3; i++)
    {
        status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
        EXPECT_STATUS (DISIR_STATUS_OK, status);

        status = dc_set_name (context_keyval, "keyval", strlen ("keyval"));
        if (i != 3)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        else
        {
            EXPECT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);
        }

        status = dc_finalize (&context_keyval);
        if (i != 3)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
            EXPECT_EQ (NULL, context_keyval);
        }
        else
        {
            // no name
            EXPECT_STATUS (DISIR_STATUS_CONTEXT_IN_WRONG_STATE, status);
            dc_putcontext (&context_keyval);
        }
    }

    dc_putcontext (&context_config);
}

