
// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"

const char *entries[] = {
    "basic_keyval",
    "basic_section",
    "json_test_mold",
    "restriction_keyval_numeric_types",
    "restriction_entries",
    "restriction_config_parent_keyval_min_entry",
    "restriction_config_parent_keyval_max_entry",
    "restriction_config_parent_section_max_entry",
    "restriction_section_parent_keyval_max_entry",
    "basic_version_difference",
    "complex_section",
    "config_query_permutations",
};

//
// This class tests the public API functions:
//  dc_compare
//
// It tests all permutations of all test entries against eachother,
// both the config and mold context'
//
class CompareParameterized :
    public ::testing::DisirTestTestPlugin,
    public ::testing::WithParamInterface<std::tuple<const char *, const char*>>
{
public:
    void compare_config (const char *entry1, const char *entry2)
    {
        struct disir_config *config1;
        struct disir_config *config2;
        struct disir_context *context_config1;
        struct disir_context *context_config2;

        status = disir_config_read (instance, "test", entry1, NULL, &config1);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        context_config1 = dc_config_getcontext (config1);

        status = disir_config_read (instance, "test", entry2, NULL, &config2);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        context_config2 = dc_config_getcontext (config2);

        status = dc_compare (context_config1, context_config2, NULL);

        if (strcmp(entry1, entry2) == 0)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        else
        {
            EXPECT_STATUS (DISIR_STATUS_CONFLICT, status);
        }


        // Cleanup
        dc_putcontext (&context_config1);
        dc_putcontext (&context_config2);
        disir_config_finished (&config1);
        disir_config_finished (&config2);
    }

    void compare_mold (const char *entry1, const char *entry2)
    {
        struct disir_mold *mold1;
        struct disir_mold *mold2;
        struct disir_context *context_mold1;
        struct disir_context *context_mold2;

        status = disir_mold_read (instance, "test", entry1, &mold1);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        context_mold1 = dc_mold_getcontext (mold1);

        status = disir_mold_read (instance, "test", entry2, &mold2);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        context_mold2 = dc_mold_getcontext (mold2);

        status = dc_compare (context_mold1, context_mold2, NULL);

        if (strcmp(entry1, entry2) == 0)
        {
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        else
        {
            EXPECT_STATUS (DISIR_STATUS_CONFLICT, status);
        }

        // Cleanup
        dc_putcontext (&context_mold1);
        dc_putcontext (&context_mold2);
        disir_mold_finished (&mold1);
        disir_mold_finished (&mold2);
    }

public:
    enum disir_status status;
};

//
// This class tests the public API functions:
//  dc_compare
//
class CompareTest : public ::testing::DisirTestTestPlugin
{
    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (context_mold1)
        {
            dc_putcontext (&context_mold1);
        }
        if (context_mold2)
        {
            dc_putcontext (&context_mold2);
        }
        if (mold1)
        {
            disir_mold_finished (&mold1);
        }
        if (mold2)
        {
            disir_mold_finished (&mold2);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_mold *mold1 = NULL;
    struct disir_mold *mold2 = NULL;
    struct disir_context *context_mold1 = NULL;
    struct disir_context *context_mold2 = NULL;
};

TEST_P (CompareParameterized, config)
{
    const char *entry1 = std::get<0>(GetParam());
    const char *entry2 = std::get<1>(GetParam());

    ASSERT_NO_FATAL_FAILURE (
        compare_config (entry1, entry2);
    );
}

TEST_P (CompareParameterized, mold)
{
    const char *entry1 = std::get<0>(GetParam());
    const char *entry2 = std::get<1>(GetParam());

    ASSERT_NO_FATAL_FAILURE (
        compare_mold (entry1, entry2);
    );
}

INSTANTIATE_TEST_CASE_P (CompareEntryCombination, CompareParameterized,
                         ::testing::Combine(::testing::ValuesIn(entries),
                                            ::testing::ValuesIn(entries)));

// Create two identical molds, except for their documentation.
TEST_F (CompareTest, mold_context_mold_documentation_content_differ)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "mold1_doc", strlen ("mold1_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_mold2, "mold2_doc", strlen ("mold2_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_mold_documentation_introduced_differ)
{
    struct disir_context *context_documentation;
    struct semantic_version semver;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "test_doc", strlen ("test_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_mold2, DISIR_CONTEXT_DOCUMENTATION, &context_documentation);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_string (context_documentation, "test_doc", strlen ("test_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    semver.sv_major = 1;
    semver.sv_minor = 1;
    semver.sv_patch = 0;
    status = dc_add_introduced (context_documentation, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_documentation);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_mold_documentation_one_missing)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "mold1_doc", strlen ("mold1_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    // test both sides
    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // test both sides
    status = dc_compare (context_mold2, context_mold1, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_default_one_missing)
{
    struct semantic_version semver;
    struct disir_context *context_keyval;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_mold2, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_keyval_boolean (context_mold1, "boolean", 0, "doc", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_boolean (context_mold2, "boolean", 0, "doc", NULL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add an extra default to keyval in mold2
    semver.sv_major = 1;
    semver.sv_minor = 2;
    semver.sv_patch = 2;
    dc_add_default_boolean (context_keyval, 1, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    dc_putcontext (&context_keyval);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    // test both sides
    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // test both sides
    status = dc_compare (context_mold2, context_mold1, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_default_introduced_differ)
{
    struct semantic_version semver;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_mold2, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_keyval_boolean (context_mold1, "boolean", 0, "doc", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    semver.sv_major = 1;
    semver.sv_minor = 2;
    semver.sv_patch = 2;
    status = dc_add_keyval_boolean (context_mold2, "boolean", 0, "doc", &semver, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_default_value_differ)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_begin (&context_mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_documentation (context_mold1, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_documentation (context_mold2, "mold_doc", strlen ("mold_doc"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_add_keyval_boolean (context_mold1, "boolean", 0, "doc", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_keyval_boolean (context_mold2, "boolean", 1, "doc", NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_mold_finalize (&context_mold1, &mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_mold_finalize (&context_mold2, &mold2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold1 = dc_mold_getcontext (mold1);
    context_mold2 = dc_mold_getcontext (mold2);

    status = dc_compare (context_mold1, context_mold2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report entry
}

TEST_F (CompareTest, mold_context_section_restriction_one_missing)
{
    struct disir_context *context_section1;
    struct disir_context *context_section2;
    struct semantic_version semver;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section1, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section2, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    semver.sv_major = 1;
    semver.sv_minor = 1;
    semver.sv_patch = 5;
    // Add equal restriction with different introduced
    status = dc_add_restriction_entries_min (context_section1, 4, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    semver.sv_minor = 2;
    status = dc_add_restriction_entries_min (context_section2, 4, &semver);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Conflict on introduced
    status = dc_compare (context_section1, context_section2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report

    dc_destroy (&context_section1);
    dc_destroy (&context_section2);
}

TEST_F (CompareTest, mold_context_section_restriction_value_differ)
{
    struct disir_context *context_section1;
    struct disir_context *context_section2;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section1, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section2, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add restriction with different value
    status = dc_add_restriction_entries_max (context_section1, 2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_restriction_entries_max (context_section2, 4, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Conflict on introduced
    status = dc_compare (context_section1, context_section2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report

    dc_destroy (&context_section1);
    dc_destroy (&context_section2);
}

TEST_F (CompareTest, mold_context_section_restriction_type_differ)
{
    struct disir_context *context_section1;
    struct disir_context *context_section2;

    ASSERT_NO_SETUP_FAILURE();

    status = dc_mold_begin (&context_mold1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section1);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_mold1, DISIR_CONTEXT_SECTION, &context_section2);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section1, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section2, "testsection", strlen ("testsection"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Add restriction with different type
    status = dc_add_restriction_entries_max (context_section1, 2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_add_restriction_entries_min (context_section2, 2, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Conflict on introduced
    status = dc_compare (context_section1, context_section2, NULL);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    // TODO: Assert report

    dc_destroy (&context_section1);
    dc_destroy (&context_section2);
}

// TODO: mold_context_mold_documentation_multiple_entries_match
// TODO: mold_context_mold_documentation_multiple_entries_one_differ
// XXX: For the above missing tests, we should update dc_add_documentation to include semver

