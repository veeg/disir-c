#include "test_json.h"

//! public disir
#include <disir/disir.h>
#include <disir/fslib/util.h>

//! mold override functions
#include "mold_override/mold_override_reference.cc"

using namespace testing;

typedef enum disir_status (*mold_apply_override)(struct disir_mold **);

//! Static map to store override reference molds by entry_id
static std::map<std::string, mold_apply_override> override_reference_molds = {
    std::make_pair ("basic_keyval", override_basic_keyval),
    std::make_pair ("json_test_mold", override_json_test_mold),
    std::make_pair ("complex_section", override_complex_section_equal_version),
    std::make_pair ("restriction_config_parent_keyval_min_entry",
                    override_restriction_config_parent_keyval_min_entry),
};

struct disir_instance * testing::JsonDioTestWrapper::instance = NULL;
struct disir_mold * testing::JsonDioTestWrapper::libdisir_mold = NULL;
struct disir_config * testing::JsonDioTestWrapper::libdisir_config = NULL;

void
testing::JsonDioTestWrapper::SetUpTestCase ()
{
    enum disir_status s;
    struct disir_context *context_config;
    struct disir_context *context_section;

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "ENTER JsonDioTestWrapper SetupTestCase");

    try
    {
    // Generate config for libdisir
    s = disir_libdisir_mold (&libdisir_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "JsonDioTestWrapper allocated libdisir mold");

    s = dc_config_begin (libdisir_mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    char path[2048];
    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_test.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "test", "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

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

    s = dc_config_set_keyval_string (context_section, "json_test", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "json_test", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "/tmp/json_test/config", "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "/tmp/json_test/mold", "mold_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_finalize (&context_config, &libdisir_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);


    s = disir_instance_create (NULL, libdisir_config, &instance);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    ASSERT_TRUE (instance != NULL);

    }
    catch (const std::exception& e)
    {
        FAIL() << "Caught exception: " << e.what() << std::endl;
    }

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "EXIT JsonDioTestWrapper SetupTestCase");
}

void
testing::JsonDioTestWrapper::TearDownTestCase ()
{
    enum disir_status status;

    if (instance)
    {
        status = disir_instance_destroy (&instance);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
    }
}

void
JsonDioTestWrapper::read_override_mold_references ()
{
    struct disir_mold *mold;
    enum disir_status status;

    for (const auto& kv : override_reference_molds)
    {
        status = disir_mold_read (instance, "test", kv.first.c_str(), &mold);
        EXPECT_STATUS (DISIR_STATUS_OK, status);

        auto override_apply_func = kv.second;

        status = override_apply_func (&mold);
        EXPECT_STATUS (DISIR_STATUS_OK, status);

        m_override_reference_molds.insert (std::make_pair (kv.first, mold));
    }
}

void
JsonDioTestWrapper::teardown_override_mold_references ()
{
    for (auto& kv : m_override_reference_molds)
    {
        disir_mold_finished (&kv.second);
    }
}

bool
JsonDioTestWrapper::GetJsonObject (Json::Value& root, std::string path)
{
    std::ifstream config (path);
    std::stringstream buffer;
    Json::Reader reader;

    if (config)
    {
        buffer << config.rdbuf();
    }
    else
    {
        return false;
    }

    bool success = reader.parse (buffer.str(), root);
    if (!success) {
        std::cerr << reader.getFormattedErrorMessages() << std::endl;
        log_test ("Could not parse jsonfile: %s",
                    reader.getFormattedErrorMessages().c_str());
        return false;
    }
    return success;
}

void
JsonDioTestWrapper::emplace_mold (const char *namespace_entry, const char *name)
{
    std::ofstream ofs;
    std::stringstream namespace_dir;
    std::stringstream mold_override;

    namespace_dir << m_mold_base_dir << namespace_entry << "/";

    status = fslib_mkdir_p (instance, namespace_dir.str().c_str());
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    std::stringstream mold_override_filepath (namespace_dir.str());
    mold_override_filepath << namespace_dir.str() << name << ".json";

    ofs.open (mold_override_filepath.str().c_str(), std::ofstream::out);
    ASSERT_TRUE (ofs.is_open());

    mold_override << m_override_entries_path << name << ".json";

    std::fstream file_override (mold_override.str());
    ASSERT_TRUE (file_override.is_open());

    ofs << file_override.rdbuf();
    ofs.close();
}

void
JsonDioTestWrapper::emplace_mold_from_string (const char *namespace_entry, const char* name,
                                              std::string& mold)
{
    std::ofstream ofs;
    std::stringstream namespace_dir;
    std::stringstream mold_override;

    namespace_dir << m_mold_base_dir << namespace_entry << "/";

    status = fslib_mkdir_p (instance, namespace_dir.str().c_str());
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    std::stringstream mold_override_filepath (namespace_dir.str());
    mold_override_filepath << namespace_dir.str() << name << ".json";

    ofs.open (mold_override_filepath.str().c_str(), std::ofstream::out);
    ASSERT_TRUE (ofs.is_open());

    ofs << mold;
    ofs.close();
}

bool
JsonDioTestWrapper::assert_invalid_context (struct disir_mold *mold, const char *name,
                                            const char *context_type, const char *errormsg)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_collection *collection;
    const char *context_name;
    status = disir_mold_valid (mold, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = DISIR_STATUS_NOT_EXIST;

    while (dc_collection_next (collection, &context) != DISIR_STATUS_EXHAUSTED)
    {
        if (strcmp (dc_context_type_string (context), context_type) == 0 &&
            ((dc_context_error (context) == NULL && errormsg == NULL) ||
            (errormsg != NULL && strcmp (dc_context_error (context), errormsg) == 0)))
        {
            if (name != NULL)
            {
                dc_get_name (context, &context_name, NULL);
                if (strcmp (context_name, name) != 0)
                    continue;

                status = DISIR_STATUS_OK;
            }
            else
            {
                status = DISIR_STATUS_OK;
            }
            goto out;
        }
    }
    // FALL-THROUGH
out:
    dc_collection_finished (&collection);

    return (DISIR_STATUS_OK == status);
}

int
JsonDioTestWrapper::invalid_context_count (struct disir_mold *mold)
{
    enum disir_status status;
    struct disir_collection *collection;

    status = disir_mold_valid (mold, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    auto invalid_contexts = dc_collection_size (collection);

    dc_collection_finished (&collection);

    return invalid_contexts;
}

