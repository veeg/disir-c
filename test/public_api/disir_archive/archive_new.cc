#include <gtest/gtest.h>
#include <disir/disir.h>
#include <disir/archive.h>
#include <stdio.h>
#include "archive_test_helper.h"
#include <archive.h>
#include <archive_entry.h>
#include <libgen.h>


//! Contains tests that test the public disir_archive_append_group API.
class ArchiveAppendNewTest : public testing::DisirTestArchive
{
    void SetUp()
    {
        DisirTestArchive::SetUp ();

        SetUpArchiveEnvironment(instance_export);

        status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();
        struct stat st;
        int ret;

        if (disir_archive)
        {
            status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
            ASSERT_STATUS (DISIR_STATUS_OK, status);
        }

        // Remove archive
        if (stat (archive_path_out, &st) == 0)
        {
            ret = remove (archive_path_out);
            ASSERT_TRUE (ret == 0);
        }

        archive = NULL;
        archive_entry = NULL;
        DisirTestArchive::TearDown ();
    }
public:
    enum disir_status status;
    struct disir_archive *disir_archive = NULL;
    struct archive *archive = NULL;
    struct archive_entry *archive_entry = NULL;
    const char *archive_path_in = "/tmp/testarchive";
    const char *archive_path_out = "/tmp/testarchive.disir";
};

TEST_F (ArchiveAppendNewTest, invalid_arguments)
{
    status = disir_archive_append_group (NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (instance_export, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (instance_export, disir_archive, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (instance_export, NULL, "JSON");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (NULL, disir_archive, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (NULL, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_group (NULL, NULL, "JSON");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (NULL, NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (instance_export, NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (instance_export, disir_archive, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (instance_export, disir_archive, "JSON", NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (NULL, disir_archive, "JSON", "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (instance_export, NULL, "JSON", "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    status = disir_archive_append_entry (instance_export, disir_archive, NULL, "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ArchiveAppendNewTest, missing_plugin_in_group)
{
    status = disir_archive_append_group (instance_export, disir_archive, "not_exist");
    EXPECT_STATUS (DISIR_STATUS_GROUP_MISSING, status);
    ASSERT_STREQ ("no plugin in group 'not_exist' available.", disir_error (instance_export));

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
}

TEST_F (ArchiveAppendNewTest, archive_created)
{
    struct stat st;

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert existence of newly created disir_archive
    ASSERT_EQ (stat (archive_path_out, &st), 0);

    // Assert file has any content at all
    ASSERT_GT (st.st_size, 0);
}

TEST_F (ArchiveAppendNewTest, group_already_exist)
{
    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_EXISTS, status);
    ASSERT_STREQ ("group 'JSON' already exist in archive", disir_error (instance_export));
}

TEST_F (ArchiveAppendNewTest, correct_archive_format)
{
    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    archive = archive_read_new();
    ASSERT_TRUE (archive != NULL);

    ASSERT_EQ (archive_read_support_format_tar (archive), ARCHIVE_OK);
    ASSERT_EQ (archive_read_support_filter_xz (archive), ARCHIVE_OK);

    archive_read_free (archive);
}

TEST_F (ArchiveAppendNewTest, append_single_entry)
{
    int ret;

    status = disir_archive_append_entry (instance_export, disir_archive, "JSON", "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    archive = archive_read_new();
    archive_read_support_format_tar (archive);
    archive_read_support_filter_xz (archive);

    ret = archive_read_open_filename (archive, archive_path_out, 10240);
    ASSERT_TRUE (ret == ARCHIVE_OK);

    while (archive_read_next_header(archive, &archive_entry) == ARCHIVE_OK) {
        std::string filename = archive_entry_pathname (archive_entry);
        if (filename.find ("metadata.toml") != std::string::npos)
            continue;
        if (filename.find ("entries.toml") != std::string::npos)
            continue;

        ASSERT_STREQ (filename.c_str(), "JSON/JSON/basic_keyval");
    }

    archive_read_free (archive);
}

TEST_F (ArchiveAppendNewTest, append_duplicate_entries)
{
    status = disir_archive_append_entry (instance_export, disir_archive, "JSON", "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_entry (instance_export, disir_archive, "JSON", "basic_keyval");
    ASSERT_STATUS (DISIR_STATUS_EXISTS, status);
    ASSERT_STREQ ("entry 'basic_keyval' already exist in archive", disir_error (instance_export));
}

TEST_F (ArchiveAppendNewTest, multiple_single_entry_appends)
{
    std::vector<std::string> insert_list {"basic_keyval", "basic_section", "complex_section",
                                          "multiple_defaults", "json_test_mold",
                                          "super/nested/basic_keyval"};
    std::vector<std::string> archive_content;
    std::vector<std::string> diff;
    std::string prefix = "/JSON/JSON/";
    int ret;

    for (auto entry : insert_list)
    {
        status = disir_archive_append_entry (instance_export, disir_archive,
                                             "JSON", entry.c_str());
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    archive = archive_read_new();
    archive_read_support_format_tar (archive);
    archive_read_support_filter_xz (archive);

    ret = archive_read_open_filename (archive, archive_path_out, 10240);
    ASSERT_TRUE (ret == ARCHIVE_OK);

    while (archive_read_next_header(archive, &archive_entry) == ARCHIVE_OK) {
        std::string filename = archive_entry_pathname (archive_entry);

        if (filename.find ("metadata.toml") != std::string::npos ||
            filename.find ("entries.toml") != std::string::npos)
        {
            continue;
        }

        // Remove prefix
        archive_content.push_back (filename.substr (prefix.length()-1, filename.length()));
    }

    std::set_difference (insert_list.begin(), insert_list.end(), archive_content.begin(),
                         archive_content.end(), std::inserter(diff, diff.begin()));

    ASSERT_TRUE (diff.empty ());
    archive_read_free (archive);
}

TEST_F (ArchiveAppendNewTest, discard_archive)
{
    struct stat st;

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Discard
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert successful clean-up
    ASSERT_NE (stat ("/tmp/archive.disir.tmp", &st), 0);
    ASSERT_NE (stat (archive_path_out, &st), 0);
    ASSERT_TRUE (disir_archive == NULL);
}

TEST_F (ArchiveAppendNewTest, finalize_with_path_to_directory)
{
    struct stat st;

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, "/tmp/lol/", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("invalid archive path: '/tmp/lol/' is a directory",
                   disir_error (instance_export));

    // Assert retry is ok
    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (stat (archive_path_out, &st), 0);
}
