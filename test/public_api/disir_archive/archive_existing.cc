#include <gtest/gtest.h>
#include <disir/disir.h>
#include <disir/archive.h>
#include <disir/fslib/util.h>
#include <stdio.h>
#include "archive_test_helper.h"
#include <archive.h>
#include <archive_entry.h>
#include <map>


//! Contains tests that test the public disir_archive_begin API with an existing archive.
class ArchiveExistingTest : public testing::DisirTestArchive
{
    void SetUp ()
    {
        DisirTestArchive::SetUp();
        SetUpArchiveEnvironment (instance_export);
        DisirLogTestBodyEnter();
    }

    void TearDown ()
    {
        DisirLogTestBodyExit();

        if (disir_archive)
        {
            disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
        }

        remove_file (archive_path_out);

        DisirTestArchive::TearDown();
    }

public:
    std::map<std::string, std::string> archive_entries;
    std::vector<std::string> config_entries;
    enum disir_status status;
    struct disir_archive *disir_archive = NULL;
    struct archive *archive = NULL;
    const char *archive_path_in = "/tmp/archive";
    const char *archive_path_out = "/tmp/archive.disir";

    const char *correct_metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";
    const char *correct_entries =
                        "[JSON]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";


    void create_metadata_toml (const char *metadata = NULL)
    {
        std::string path = "metadata.toml";
        if (metadata)
            archive_entries.insert (std::pair<std::string, std::string> (path, metadata));
        else
            archive_entries.insert (std::pair<std::string, std::string> (path, correct_metadata));
    }

    void create_entries_toml (const char *subfolder, const char *entries = NULL)
    {
        std::string path = subfolder;
        path = path + "/entries.toml";
        if (entries)
            archive_entries.insert (std::pair<std::string, std::string> (path, entries));
        else
            archive_entries.insert (std::pair<std::string, std::string> (path, correct_entries));
    }

    void create_config_entries (const char *subfolder)
    {
        std::string path = subfolder;

        path = path + "/";

        if (!config_entries.empty())
        {
            for (const auto& config : config_entries)
            {
                archive_entries.insert (std::pair<std::string, std::string> (path +
                                        config, "some content"));
            }
        }
        // Create default
        else
        {
            archive_entries.insert (std::pair<std::string, std::string> (path +
                                    "basic_keyval", "some content"));
            archive_entries.insert (std::pair<std::string, std::string> (path +
                                    "basic_section", "some content"));
            archive_entries.insert (std::pair<std::string, std::string> (path +
                                    "nested/basic_keyval", "some content"));
            archive_entries.insert (std::pair<std::string, std::string> (path +
                                    "super/nested/basic_keyval", "some content"));
        }
    }

    void create_mockup_archive ()
    {
        const char *metadata =
                            "disir_org_version = \"0/1-draft\"\n"
                            "implementation = \"0.1.0\"\n"
                            "\n"
                            "[[backend]]\n"
                            "  groups = [\"Random\"]\n"
                            "  id = \"Random Config Test\"";
        const char *entries =
                            "[Random]\n"
                            "  basic_keyval = \"1.0.0\"\n"
                            "  basic_section = \"1.0.0\"\n"
                            "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                            "  \"super/nested/basic_keyval\" = \"1.0.0\"";

        create_metadata_toml (metadata);
        create_entries_toml ("Random Config Test", entries);
        create_config_entries ("Random Config Test/Random");
        create_archive();
    }

    void create_archive ()
    {
        int ret;
        struct archive_entry *archive_entry;
        FILE *out_archive;

        archive = archive_write_new ();
        ASSERT_TRUE (archive != NULL);

        ret = archive_write_set_format (archive, ARCHIVE_FORMAT_TAR);
        ASSERT_TRUE (ret == ARCHIVE_OK);

        out_archive = fopen (archive_path_out, "w+");
        ASSERT_TRUE (out_archive != NULL);

        archive_write_add_filter_xz (archive);
        archive_write_open_FILE (archive, out_archive);

        // Loop map
        for (const auto& entry : archive_entries)
        {
            archive_entry = archive_entry_new();
            ASSERT_TRUE (archive_entry != NULL);

            archive_entry_set_pathname (archive_entry, entry.first.c_str());
            archive_entry_set_size (archive_entry, entry.second.length());
            archive_entry_set_filetype (archive_entry, AE_IFREG);
            archive_entry_set_perm (archive_entry, 0644);
            archive_write_header (archive, archive_entry);
            ret = archive_write_data (archive, entry.second.c_str(), entry.second.length());
            ASSERT_TRUE (ret >= 0);
            archive_write_finish_entry (archive);
            archive_entry_free (archive_entry);
        }

        archive_write_close (archive);
        archive_write_free (archive);
        fclose (out_archive);
    }

    void remove_file (const char *archive_path)
    {
        struct stat st;
        int ret;

        if (stat (archive_path, &st) == 0)
        {
            ret = remove (archive_path);
            ASSERT_TRUE (ret == 0);
        }
    }

    void get_archive_content (const char *archive_path, std::vector<std::string>& content)
    {
        int ret;
        struct archive *archive;
        struct archive_entry *archive_entry;
        std::vector<std::string> archive_content;

        archive = archive_read_new();
        archive_read_support_format_tar (archive);
        archive_read_support_filter_xz (archive);

        ret = archive_read_open_filename (archive, archive_path, 10240);
        ASSERT_TRUE (ret == ARCHIVE_OK);

        while (archive_read_next_header(archive, &archive_entry) == ARCHIVE_OK) {
            content.push_back (std::string (archive_entry_pathname (archive_entry)));
        }

        archive_read_free (archive);
    }


    void archive_diff (std::vector<std::string> lhs, std::vector<std::string> rhs,
                                                     std::vector<std::string>& diff)
    {
        std::sort(lhs.begin(), lhs.end());
        std::sort(rhs.begin(), rhs.end());

        if (lhs.size() > rhs.size())
            std::set_difference (lhs.begin(), lhs.end(), rhs.begin(),
                                 rhs.end(), std::inserter(diff, diff.begin()));
        else
            std::set_difference (rhs.begin(), rhs.end(), lhs.begin(),
                                 lhs.end(), std::inserter(diff, diff.begin()));
    }

    bool has_extension (const char *archive_path)
    {
        const char *ext = NULL;
        // Assert that archive contains the .disir extension
        ext = strrchr (archive_path, '.');
        if (ext == NULL || strcmp (ext, ".disir") != 0)
        {
            return false;
        }

        return true;
    }
};

TEST_F (ArchiveExistingTest, invalid_arguments)
{
    status = disir_archive_export_begin (NULL, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_archive_export_begin (instance_export, NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_archive_export_begin (instance_export, "/tmp/path", NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_archive_export_begin (NULL, "/tmp/path", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (ArchiveExistingTest, invalid_filepath)
{
    FILE *invalid;

    // Directory given
    status = disir_archive_export_begin (instance_export, "/tmp/dir_path_given/", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
    ASSERT_STREQ ("invalid archive path: input path is a directory",
                  disir_error (instance_export));

    // Non-existing file given
    status = disir_archive_export_begin (instance_export, "/tmp/no_file.disir", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("unable to read archive: '/tmp/no_file.disir'", disir_error (instance_export));

    // File without .disir extension given:
    invalid = fopen ("/tmp/no_ext", "w");
    fclose (invalid);

    status = disir_archive_export_begin (instance_export, "/tmp/no_ext", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    remove_file ("/tmp/no_ext");
}

TEST_F (ArchiveExistingTest, no_metadata_toml)
{
    create_entries_toml ("JSON Config Test");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, invalid_metadata_toml_format)
{
    const char *metadata =
                        "disir_org_version  \"0/1-draft\"\n" // Invalid line
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, missing_disir_org_version_entry)
{
    const char *metadata =
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, disir_org_version_entry_not_string)
{
    const char *metadata =
                        "disir_org_version = 2\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
}

TEST_F (ArchiveExistingTest, disir_org_version_differ_from_system)
{
    const char *metadata =
                        "disir_org_version = \"512/22-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
}

TEST_F (ArchiveExistingTest, missing_implementation_entry)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, implementation_entry_not_string)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = 5\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, disir_implementation_version_differ_from_system)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"512.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, no_backends_present)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n";

    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, backend_not_array)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[backend]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = \"JSON Config Test\"";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, id_not_present_in_backend)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, value_of_id_not_string)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\"]\n"
                        "  id = 2";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, groups_key_not_present_in_backend)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  id = \"JSON Config Test\"";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, groups_is_not_array)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = \"JSON\"\n"
                        "  id = \"JSON Config Test\"";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, empty_groups_array)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = []\n"
                        "  id = \"JSON Config Test\"";
    create_metadata_toml (metadata);
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, no_entries_toml)
{
    create_metadata_toml();
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, invalid_entries_toml_format)
{
    const char *entries =
                        "[JSON\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";
    create_metadata_toml();
    create_entries_toml ("JSON Config Test", entries);
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, group_not_matching_metadata)
{
    const char *entries =
                        "[WRONG]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";
    create_metadata_toml();
    create_entries_toml ("JSON Config Test", entries);
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, missing_groups)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\", \"WRONG\"]\n"
                        "  id = \"JSON Config Test\"";

    create_metadata_toml (metadata);
    create_entries_toml ("JSON Config Test");
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, group_not_toml_table)
{
    const char *entries = "JSON = \"wrong\"\n";

    create_metadata_toml();
    create_entries_toml ("JSON Config Test", entries);
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_WRONG_VALUE_TYPE, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, config_not_in_map)
{
    const char *entries =
                        "[JSON]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";

    config_entries.push_back("basic_section");
    config_entries.push_back("nested/basic_keyval");
    config_entries.push_back("super/nested/basic_keyval");

    create_metadata_toml();
    create_entries_toml ("JSON Config Test", entries);
    create_config_entries ("JSON Config Test/JSON");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_STREQ ("invalid archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, multiple_backends_multiple_groups)
{
    const char *metadata =
                        "disir_org_version = \"0/1-draft\"\n"
                        "implementation = \"0.1.0\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"JSON\", \"meos\", \"kspt\"]\n"
                        "  id = \"JSON Config Test\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"NOT_JSON\", \"meos\"]\n"
                        "  id = \"NOT JSON Config Test\"\n"
                        "\n"
                        "[[backend]]\n"
                        "  groups = [\"kspt\"]\n"
                        "  id = \"RandomTest\"";
    const char *entries1 =
                        "[JSON]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\""
                        "\n"
                        "[meos]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\""
                        "\n"
                        "[kspt]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";
    const char *entries2 =
                        "[NOT_JSON]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\""
                        "\n"
                        "[meos]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";

    const char *entries3 =
                        "[kspt]\n"
                        "  basic_keyval = \"1.0.0\"\n"
                        "  basic_section = \"1.0.0\"\n"
                        "  \"nested/basic_keyval\" = \"1.0.0\"\n"
                        "  \"super/nested/basic_keyval\" = \"1.0.0\"";

    create_metadata_toml (metadata);
    create_entries_toml ("JSON Config Test", entries1);
    create_entries_toml ("NOT JSON Config Test", entries2);
    create_entries_toml ("RandomTest", entries3);
    create_config_entries ("JSON Config Test/JSON");
    create_config_entries ("JSON Config Test/meos");
    create_config_entries ("JSON Config Test/kspt");
    create_config_entries ("NOT JSON Config Test/NOT_JSON");
    create_config_entries ("NOT JSON Config Test/meos");
    create_config_entries ("RandomTest/kspt");
    create_archive();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ArchiveExistingTest, append_to_existing)
{
    create_mockup_archive ();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (ArchiveExistingTest, append_existing_group)
{
    create_mockup_archive ();

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_EXISTS, status);
}

TEST_F (ArchiveExistingTest, discard_append)
{
    std::vector<std::string> old_content;
    std::vector<std::string> new_content;
    std::vector<std::string> diff;

    create_mockup_archive ();

    get_archive_content (archive_path_out, old_content);

    // Begin existing
    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Append new group
    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Discard append
    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    get_archive_content (archive_path_out, new_content);

    archive_diff (new_content, old_content, diff);

    ASSERT_TRUE (diff.empty ());
}

TEST_F (ArchiveExistingTest, archive_append_overwrite_existing)
{
    std::vector<std::string> old_content;
    std::vector<std::string> new_content;
    std::vector<std::string> diff;

    create_mockup_archive ();

    get_archive_content (archive_path_out, old_content);

    // Begin existing
    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Append new group
    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Overwrite existing archive
    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    get_archive_content (archive_path_out, new_content);

    archive_diff (new_content, old_content, diff);

    ASSERT_FALSE (diff.empty ());
}

TEST_F (ArchiveExistingTest, archive_append_new_output_location)
{
    int ret;
    struct stat st;
    std::vector<std::string> old_content;
    std::vector<std::string> new_content;
    std::vector<std::string> diff;
    const char *new_archive_path_in = "/tmp/newfolder/archive";
    const char *new_archive_path_out = "/tmp/newfolder/archive.disir";

    create_mockup_archive ();

    get_archive_content (archive_path_out, old_content);

    // Begin existing
    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Append new group
    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Create new folder to extract archive
    status = fslib_mkdir_p (instance_export, "/tmp/newfolder");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Overwrite existing archive
    status = disir_archive_finalize (instance_export, new_archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    get_archive_content (new_archive_path_out, new_content);

    archive_diff (new_content, old_content, diff);

    ASSERT_FALSE (diff.empty ());
    ASSERT_EQ (stat (new_archive_path_out, &st), 0);
    ASSERT_NE (stat (archive_path_out, &st), 0);

    // Remove archive and folder
    remove_file (new_archive_path_out);
    ret = remove ("/tmp/newfolder");
    ASSERT_EQ (ret, 0);
}

TEST_F (ArchiveExistingTest, append_entry)
{
    std::vector<std::string> new_content;
    std::vector<std::string> old_content;
    std::vector<std::string> diff;
    std::string prefix = "JSON/JSON/";
    std::vector<std::string> insert_list {"basic_keyval", "basic_section", "complex_section",
                                          "multiple_defaults", "json_test_mold",
                                          "super/nested/basic_keyval"};
    // Begin new
    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Append some entries
    for (auto entry : insert_list)
    {
        status = disir_archive_append_entry (instance_export, disir_archive,
                                             "JSON", entry.c_str());
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    // Finalize it
    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Set prefixes
    for (auto entry : insert_list)
    {
        old_content.push_back (entry.insert (0, prefix));
    }

    // Re-open same archive
    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Append some more
    status = disir_archive_append_entry (instance_export, disir_archive,
                                         "JSON", "config_query_permutations");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_entry (instance_export, disir_archive,
                                         "JSON", "restriction_config_parent_keyval_max_entry");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    get_archive_content (archive_path_out, new_content);

    archive_diff (old_content, new_content, diff);

    ASSERT_TRUE (diff.size() == 4); // 2 entries + metadata.toml and entries.toml
    ASSERT_TRUE (std::find(diff.begin(), diff.end(), "JSON/JSON/config_query_permutations")
                 != diff.end());
    ASSERT_TRUE (std::find(diff.begin(), diff.end(),
                           "JSON/JSON/restriction_config_parent_keyval_max_entry") != diff.end());
}

TEST_F (ArchiveExistingTest, missing_disir_extension)
{
    int ret;

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ret = rename ("/tmp/archive.disir", "/tmp/noextension");
    ASSERT_TRUE (ret == 0);

    status = disir_archive_export_begin (instance_export, "/tmp/noextension", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
    ASSERT_STREQ ("input archive is not a disir archive", disir_error (instance_export));

    remove_file ("/tmp/noextension");
}

TEST_F (ArchiveExistingTest, multiple_file_extensions)
{
    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, "/tmp/my.file.with.extensions",
                                     &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Test if multiple extensions is parsed and asserted correctly.
    // Output file is really named "my.file.with.extensions.disir", but nameparser will
    // catch the missing extension first.
    status = disir_archive_export_begin (instance_export, "/tmp/my.file.with.extensions",
                                         &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);

    remove_file ("/tmp/my.file.with.extensions.disir");
}

TEST_F (ArchiveExistingTest, empty_archive)
{
    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_finalize (instance_export, "/tmp/empty", &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_NO_CAN_DO, status);
    ASSERT_STREQ ("cannot finalize empty archive", disir_error (instance_export));
}

TEST_F (ArchiveExistingTest, not_permitted_to_write_archive)
{
    struct stat st;
    int ret;
    size_t old_mode = 0;

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = fslib_mkdir_p (instance_export, "/tmp/restricted_folder");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ret = stat ("/tmp/restricted_folder", &st);
    ASSERT_TRUE (ret == 0);

    old_mode = st.st_mode;

    // Remove write permission in extract folder
    ret = chmod ("/tmp/restricted_folder", old_mode ^ S_IWUSR);
    ASSERT_TRUE (ret == 0);

    status = disir_archive_finalize (instance_export, "/tmp/restricted_folder/archive",
                                     &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_PERMISSION_ERROR, status);
    ASSERT_STREQ ("unable to write to path: '/tmp/restricted_folder'",
                   disir_error (instance_export));

    status = disir_archive_finalize (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ret = remove ("/tmp/restricted_folder");
    ASSERT_EQ (ret, 0);
}

TEST_F (ArchiveExistingTest, no_write_permission_continue_from_error)
{
    struct stat st;
    int ret;
    size_t old_mode = 0;

    status = disir_archive_export_begin (instance_export, NULL, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = disir_archive_append_group (instance_export, disir_archive, "JSON");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = fslib_mkdir_p (instance_export, "/tmp/restricted_folder");
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ret = stat ("/tmp/restricted_folder", &st);
    ASSERT_TRUE (ret == 0);

    old_mode = st.st_mode;

    // Remove write permission in extract folder
    ret = chmod ("/tmp/restricted_folder", old_mode ^ S_IWUSR);
    ASSERT_TRUE (ret == 0);

    status = disir_archive_finalize (instance_export, "/tmp/restricted_folder/archive",
                                     &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_PERMISSION_ERROR, status);
    ASSERT_STREQ ("unable to write to path: '/tmp/restricted_folder'",
                   disir_error (instance_export));

    // Retry finalize to permitted path
    status = disir_archive_finalize (instance_export, archive_path_in, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Assert archive was written
    ASSERT_EQ (stat (archive_path_out, &st), 0);

    ret = remove ("/tmp/restricted_folder");
    ASSERT_EQ (ret, 0);
}

TEST_F (ArchiveExistingTest, not_permitted_to_read_archive)
{
    struct stat st;
    int ret;
    size_t old_mode;

    create_mockup_archive ();

    ret = stat (archive_path_out, &st);
    ASSERT_TRUE (ret == 0);

    old_mode = st.st_mode;

    // Remove read permission on archive
    ret = chmod (archive_path_out, old_mode ^ S_IRUSR);
    ASSERT_TRUE (ret == 0);

    status = disir_archive_export_begin (instance_export, archive_path_out, &disir_archive);
    ASSERT_STATUS (DISIR_STATUS_PERMISSION_ERROR, status);

    ASSERT_STREQ ("unable to read archive: '/tmp/archive.disir'", disir_error (instance_export));
}
