#include <sstream>
#include <experimental/filesystem>

#include "archive_test_helper.h"

#include "config_overrides/multiple_defaults_override.cc"
#include "config_overrides/json_test_mold_override.cc"

struct disir_instance * testing::DisirTestArchive::instance_import = NULL;
struct disir_instance * testing::DisirTestArchive::instance_export = NULL;
struct disir_mold * testing::DisirTestArchive::libdisir_mold_import = NULL;
struct disir_mold * testing::DisirTestArchive::libdisir_mold_export = NULL;
struct disir_config * testing::DisirTestArchive::libdisir_config_import = NULL;
struct disir_config * testing::DisirTestArchive::libdisir_config_export = NULL;

typedef enum disir_status (*config_apply_override)(struct disir_mold*, struct disir_config **);

//! Static map to store config overrides
static 
std::map<std::string, std::tuple<std::string, config_apply_override>> override_reference_configs = {
    std::make_pair ("multiple_defaults_1_2",
                    std::make_tuple("multiple_defaults", multiple_defaults_override_1_2)),
    std::make_pair ("multiple_defaults_1_0",
                    std::make_tuple("multiple_defaults", multiple_defaults_override_1_0)),
    std::make_pair ("multiple_defaults_1_0_restriction",
                    std::make_tuple("multiple_defaults",
                                    multiple_defaults_1_0_restriction)),
    std::make_pair ("multiple_defaults_1_2_differ_1_0",
                    std::make_tuple("multiple_defaults",
                                    multiple_defaults_1_2_differ_1_0)),
    std::make_pair ("json_test_mold_2_0",
                    std::make_tuple("json_test_mold", json_test_mold_override)),
};

void
testing::DisirTestArchive::GenerateAllEntries (struct disir_instance *instance,
                                               std::set<std::string>& entries,
                                               const char *group_name)
{
    enum disir_status status;
    struct disir_mold *mold;
    struct disir_config *config;

    for (const auto entry : entries)
    {
        status = disir_mold_read (instance, "test", entry.c_str(), &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_generate_config_from_mold (mold, NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_mold_write (instance, group_name, entry.c_str(), mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_config_write (instance, group_name, entry.c_str(), config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        disir_config_finished (&config);
        disir_mold_finished (&mold);
    }
}

void
testing::DisirTestArchive::SetUpArchiveEnvironment (struct disir_instance *instance)
{
    enum disir_status status;
    struct disir_entry *mold_entries;
    struct disir_entry *next;
    struct disir_entry *current;
    std::set<std::string> entries_to_generate;

    // Find available configs
    mold_entries = NULL;
    status = disir_mold_entries (instance, "test", &mold_entries);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    current = mold_entries;
    while (current != NULL)
    {
        next = current->next;
        if (current->flag.DE_NAMESPACE_ENTRY)
        {
            disir_entry_finished (&current);
            current = next;
            continue;
        }

        status = disir_config_query (instance, "test", current->de_entry_name, NULL);
        if (status == DISIR_STATUS_EXISTS)
        {
            entries_to_generate.insert (std::string(current->de_entry_name));
        }

        disir_entry_finished (&current);
        current = next;
    }

    // Generate
    if (!entries_to_generate.empty ())
    {
        GenerateAllEntries (instance, entries_to_generate, "JSON");
    }
}

void
testing::DisirTestArchive::SetUpTestCase ()
{
    enum disir_status s;
    struct disir_context *context_config;
    struct disir_context *context_section;

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "ENTER DisirTestTestPlugin SetupTestCase");

    try
    {
    // Generate config for libdisir
    s = disir_libdisir_mold (&libdisir_mold_export);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "DisirTestTestPlugin allocated libdisir mold");

    s = dc_config_begin (libdisir_mold_export, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    char path[2048];
    char config_path[2048];

    strcpy (config_path, "/tmp/export/test_configs");

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_test.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);


    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_json.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "JSON", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/export/test_molds");

    s = dc_config_set_keyval_string (context_section, config_path, "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/export/test_configs");

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_json.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "JSON_EXPORT", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "JSON_EXPORT", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/export/test_molds");
    s = dc_config_set_keyval_string (context_section, config_path, "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/export/test_configs");

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_json.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "mismatch", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "mismatch", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/export/test_mold");
    s = dc_config_set_keyval_string (context_section, config_path, "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_finalize (&context_config, &libdisir_config_export);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = disir_instance_create (NULL, libdisir_config_export, &instance_export);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    ASSERT_TRUE (instance_export != NULL);

    // Import instance
    s = disir_libdisir_mold (&libdisir_mold_import);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_begin (libdisir_mold_import, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_json.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "JSON", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/import/test_configs");

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/import/test_molds");

    s = dc_config_set_keyval_string (context_section, config_path, "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/import/test_configs");

    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_test.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    strcpy (config_path, "/tmp/import/test_configs");

    // Equal group id but backend_ids differ
    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_test.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "mismatch", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "mismatch", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, config_path, "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_finalize (&context_config, &libdisir_config_import);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = disir_instance_create (NULL, libdisir_config_import, &instance_import);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    ASSERT_TRUE (instance_import != NULL);

    }
    catch (const std::exception& e)
    {
        FAIL() << "Caught exception: " << e.what() << std::endl;
    }

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "EXIT DisirTestTestPlugin SetupTestCase");
}

void
testing::DisirTestArchive::TearDownTestCase ()
{
    if (instance_export)
    {
        enum disir_status s = disir_instance_destroy (&instance_export);
        EXPECT_STATUS (DISIR_STATUS_OK, s);
    }
    if (instance_import)
    {
        enum disir_status s = disir_instance_destroy (&instance_import);
        EXPECT_STATUS (DISIR_STATUS_OK, s);
    }


    try
    {
        if (std::experimental::filesystem::exists ("/tmp/import/"))
        {
            std::experimental::filesystem::remove_all ("/tmp/import/");
        }
        if (std::experimental::filesystem::exists ("/tmp/export/"))
        {
            std::experimental::filesystem::remove_all ("/tmp/export/");
        }
    }
    catch (const std::exception &e)
    {
       //FAIL() << "Caught exception: " << e.what() << std::endl;
    }
}

void
testing::DisirTestArchive::GenerateConfigOverrides ()
{
    struct disir_config *config;
    struct disir_mold *mold;

    for (const auto& kv : override_reference_configs)
    {
        status = disir_mold_read (instance_import, "test", std::get<0>(kv.second).c_str(), &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        auto override_apply_func = std::get<1>(kv.second);

        status = override_apply_func (mold, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        m_nondefault_configs.insert (std::make_pair (kv.first, config));

        disir_mold_finished (&mold);
    }
}

void
testing::DisirTestArchive::TeardownConfigOverrides()
{
    for (auto& kv : m_nondefault_configs)
    {
        status = disir_config_finished (&kv.second);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
    }
}

