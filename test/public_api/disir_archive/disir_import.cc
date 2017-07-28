#include <gtest/gtest.h>
#include <experimental/filesystem>

// PUBLIC API
#include <disir/disir.h>
#include <disir/archive.h>

#include "archive_test_helper.h"

class ImportTest : public testing::DisirTestArchive
{
    void SetUp ()
    {
        DisirTestArchive::SetUp ();

        DisirLogTestBodyEnter ();

        GenerateConfigOverrides();
        status = disir_archive_export_begin (instance_export, NULL, &archive);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void TearDown ()
    {
        if (archive)
        {
            status = disir_archive_finalize (instance_export, "/tmp/archive.disir", &archive);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }

        TeardownConfigOverrides();

        if (import)
        {
            status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }
        // Remove all configs and molds
        if (std::experimental::filesystem::exists ("/tmp/import/test_configs/"))
        {
            std::experimental::filesystem::remove_all ("/tmp/import/test_configs/");
        }
        if (std::experimental::filesystem::exists ("/tmp/archive.disir"))
        {
            std::experimental::filesystem::remove ("/tmp/archive.disir");
        }
        if (std::experimental::filesystem::exists ("/tmp/import/test_molds/"))
        {
            std::experimental::filesystem::remove_all ("/tmp/import/test_molds/");
        }
        DisirLogTestBodyExit ();

        DisirTestArchive::TearDown ();
    }

public:
    void setup_export_import_config (const char *entry, const char *entry_override,
                                     bool mold_support, struct disir_config *config_at_import)
    {
        struct disir_config *config = NULL;
        status = disir_mold_read (instance_export, "test", entry, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_mold_write (instance_export, "JSON", entry, mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        // place mold at instance import
        if (mold_support)
        {
            status = disir_mold_write (instance_import, "JSON", entry, mold);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }

        config = m_nondefault_configs[entry_override];

        status = disir_config_write (instance_export, "JSON", entry, config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        // have an equivalent config at import instance
        if (config_at_import)
        {
            status = disir_config_write (instance_import, "JSON", entry, config_at_import);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }

        status = disir_archive_append_entry (instance_export, archive, "JSON", entry);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        disir_mold_finished (&mold);
    }

    void compare_imported_with_ref (const char *entry, struct disir_config *config_ref)
    {
        struct disir_config *config;
        struct disir_context *context_config_ref;
        struct disir_context *context_config_imported;
        struct disir_diff_report *report;

        status = disir_config_read (instance_import, "JSON", entry, NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        context_config_ref = dc_config_getcontext (config);
        ASSERT_TRUE (context_config_ref != NULL);
        context_config_imported = dc_config_getcontext (config_ref);
        ASSERT_TRUE (context_config_imported != NULL);

        status = dc_compare (context_config_ref, context_config_imported, &report);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        dc_putcontext (&context_config_ref);
        dc_putcontext (&context_config_imported);
        disir_config_finished (&config);
    }
    void finalize_export_begin_import ()
    {
        status = disir_archive_finalize (instance_export, "/tmp/archive.disir", &archive);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_archive_import (instance_import, "/tmp/archive.disir",
                                       &import, &import_entries);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

public:
    int import_entries;
    enum disir_status status;
    struct disir_archive *archive;
    struct disir_mold *mold = NULL;
    struct disir_import *import = NULL;
    const char *entry_id;
    const char *group_id;
    const char *version;
    const char *errmsg;
};

TEST_F (ImportTest, import_archive_not_exist)
{
    setup_export_import_config ("json_test_mold", "json_test_mold_2_0", false, NULL);

    status = disir_archive_finalize (instance_export, "/tmp/archive.disir", &archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_import (instance_import, "/tmp/invalid",
                                   &import, &import_entries);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
}

TEST_F (ImportTest, import_no_mold_support_shall_fail)
{
    setup_export_import_config ("json_test_mold", "json_test_mold_2_0", false, NULL);
    finalize_export_begin_import ();

    ASSERT_EQ (1, import_entries);

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    ASSERT_STREQ ("json_test_mold", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("2.0", version);
    ASSERT_STREQ ("system does not recognize archive entry", errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DISCARD);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
}

TEST_F (ImportTest, import_no_mold_support_invalid_options)
{
    setup_export_import_config ("json_test_mold", "json_test_mold_2_0",false, NULL);
    finalize_export_begin_import ();

    ASSERT_EQ (1, import_entries);

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    ASSERT_STREQ ("json_test_mold", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("2.0", version);
    ASSERT_STREQ ("system does not recognize archive entry", errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_FORCE);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DO);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE_WITH_DISCARD);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
}

TEST_F (ImportTest, import_config_conflicts_with_existing_force)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_0", true,
                                m_nondefault_configs["multiple_defaults_1_2"]);
    finalize_export_begin_import();

    ASSERT_EQ (1, import_entries);

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("version of existing entry (1.2) is higher than archive entry (1.0)",
                  errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_FORCE);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_0"]);
    );
}

TEST_F (ImportTest, import_config_conflicts_with_existing_soft)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_0", true,
                                m_nondefault_configs["multiple_defaults_1_2_differ_1_0"]);
    finalize_export_begin_import();

    ASSERT_EQ (1, import_entries);

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("version of existing entry (1.2) is higher than archive entry (1.0)",
                  errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_2"]);
    );
}

TEST_F (ImportTest, import_config_conflicts_with_existing_discard)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_2_differ_1_0", true,
                                m_nondefault_configs["multiple_defaults_1_0"]);
    finalize_export_begin_import();

    ASSERT_EQ (1, import_entries);

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICT, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.2", version);
    ASSERT_STREQ ("version of archive entry (1.2) is higher than existing entry (1.0)",
                  errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DISCARD);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_0"]);
    );
}

TEST_F (ImportTest, import_config_discard_shall_not_exist)
{
    struct disir_config *config = NULL;
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_2", true, NULL);
    finalize_export_begin_import();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.2", version);
    ASSERT_TRUE (errmsg == NULL);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DISCARD);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_config_read (instance_import, "JSON", "multiple_defaults", NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    disir_config_finished (&config);
}

TEST_F (ImportTest, import_imported_config_version_higher_than_existing)
{
    setup_export_import_config ("json_test_mold", "json_test_mold_2_0", true, NULL);
    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_STREQ ("json_test_mold", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("2.0", version);
    ASSERT_TRUE (errmsg == NULL);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DO);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("json_test_mold", m_nondefault_configs["json_test_mold_2_0"]);
    );
}

TEST_F (ImportTest, import_no_config_equiv_conflict)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_2", true, NULL);
    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.2", version);
    ASSERT_TRUE (errmsg == NULL);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DO);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_2"]);
    );
}

TEST_F (ImportTest, import_higher_mold_version_update)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_0", true, NULL);
    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICTING_SEMVER, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("archive entry version is (1.0) but system version is (1.2)", errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_2"]);
    );
}

TEST_F (ImportTest, import_higher_mold_version_import_do)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_0", true, NULL);
    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICTING_SEMVER, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("archive entry version is (1.0) but system version is (1.2)", errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_DO);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_0"]);
    );
}

TEST_F (ImportTest, import_higher_mold_version_restriction_violated)
{
    setup_export_import_config ("multiple_defaults", "multiple_defaults_1_0_restriction",
                                true, NULL);
    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_CONFLICTING_SEMVER, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("archive entry version is (1.0) but system version is (1.2)", errmsg);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE);
    ASSERT_STATUS (DISIR_STATUS_RESTRICTION_VIOLATED, status);

    status = disir_import_resolve_entry (import, 0, DISIR_IMPORT_UPDATE_WITH_DISCARD);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_import_finalize (instance_import, DISIR_IMPORT_DO, &import, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_imported_with_ref ("multiple_defaults",
                                   m_nondefault_configs["multiple_defaults_1_2"]);
    );
}

TEST_F (ImportTest, import_config_with_higher_version_than_mold)
{
    struct disir_context *context_mold;
    struct disir_config *config = NULL;
    struct disir_mold *mold;
    struct disir_version ver;

    status = disir_mold_read (instance_export, "test", "multiple_defaults", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Write mold at version 1.2.0 to import instance
    status = disir_mold_write (instance_import, "JSON", "multiple_defaults", mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_mold = dc_mold_getcontext (mold);
    ASSERT_TRUE (context_mold != NULL);

    ver = {2,0};
    status = dc_add_keyval_string (context_mold, "keyval_2_0", "2_0",
                                   "doc", &ver, NULL);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    dc_putcontext (&context_mold);

    // Write mold at version 2.0.0 to export
    status = disir_mold_write (instance_export, "JSON", "multiple_defaults", mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_generate_config_from_mold (mold, &ver, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_config_write (instance_export, "JSON", "multiple_defaults", config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    disir_mold_finished (&mold);
    disir_config_finished (&config);

    status = disir_archive_append_entry (instance_export, archive, "JSON", "multiple_defaults");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    finalize_export_begin_import ();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON", group_id);
    ASSERT_STREQ ("2.0", version);
    ASSERT_STREQ ("archive entry is newer than system version", errmsg);
}

TEST_F (ImportTest, import_group_not_on_import_instance)
{
    struct disir_config *config = NULL;

    status = disir_mold_read (instance_export, "test", "multiple_defaults", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_mold_write (instance_export, "JSON_EXPORT", "multiple_defaults", mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    config = m_nondefault_configs["multiple_defaults_1_0"];

    status = disir_config_write (instance_export, "JSON_EXPORT", "multiple_defaults", config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_entry (instance_export, archive,
                                         "JSON_EXPORT", "multiple_defaults");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    finalize_export_begin_import();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("JSON_EXPORT", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("group 'JSON_EXPORT' is not supported by system", errmsg);

    disir_mold_finished (&mold);
}

TEST_F (ImportTest, import_equal_group_yet_no_equal_backend_name)
{
    struct disir_config *config = NULL;

    status = disir_mold_read (instance_export, "test", "multiple_defaults", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_mold_write (instance_export, "mismatch", "multiple_defaults", mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    config = m_nondefault_configs["multiple_defaults_1_0"];

    status = disir_config_write (instance_export, "mismatch", "multiple_defaults", config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_entry (instance_export, archive, "mismatch", "multiple_defaults");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    finalize_export_begin_import();

    status = disir_import_entry_status (import, 0, &entry_id, &group_id, &version, &errmsg);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    ASSERT_STREQ ("multiple_defaults", entry_id);
    ASSERT_STREQ ("mismatch", group_id);
    ASSERT_STREQ ("1.0", version);
    ASSERT_STREQ ("mismatch between groups on system", errmsg);

    disir_mold_finished (&mold);
}

