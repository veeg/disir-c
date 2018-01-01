#include <sstream>

#include "test_helper.h"


struct disir_instance * testing::DisirTestTestPlugin::instance = NULL;
struct disir_mold * testing::DisirTestTestPlugin::libdisir_mold = NULL;
struct disir_config * testing::DisirTestTestPlugin::libdisir_config = NULL;


void
testing::DisirTestWrapper::DisirLogCurrentTest (const char *prefix)
{
    // Log current test running
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    if (test_info != NULL)
    {
        _log_disir_level (DISIR_LOG_LEVEL_TEST, "%s %s.%s", prefix,
                          test_info->test_case_name(), test_info->name());
    }
}

void
testing::DisirTestWrapper::DisirLogCurrentTestEnter ()
{
   DisirLogCurrentTest ("ENTERING TEST SETUP");
}

void
testing::DisirTestWrapper::DisirLogCurrentTestExit ()
{
    DisirLogCurrentTest ("EXITING TEST TEARDOWN");
}

void
testing::DisirTestWrapper::DisirLogTestBodyEnter ()
{
    DisirLogCurrentTest ("ENTERING TEST BODY");
}

void
testing::DisirTestWrapper::DisirLogTestBodyExit ()
{
    DisirLogCurrentTest ("EXITING TEST BODY");
}

void
testing::DisirTestTestPlugin::SetUpTestCase ()
{

    enum disir_status s;
    struct disir_context *context_config;
    struct disir_context *context_section;
    char path[2048];

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "ENTER DisirTestTestPlugin SetupTestCase");

    try
    {
    // Generate config for libdisir
    s = disir_libdisir_mold (&libdisir_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "DisirTestTestPlugin allocated libdisir mold");

    s = dc_config_begin (libdisir_mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    path[0] = '\0';
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

    // Finalize plugin section
    s = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    // Open a plugin section
    s = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_section, "plugin", strlen ("plugin"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    path[0] = '\0';
    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugins/dplugin_json.so");
    s = dc_config_set_keyval_string (context_section, path, "plugin_filepath");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "json", "io_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section, "json", "group_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_config_set_keyval_string (context_section,
                                     CMAKE_BUILD_DIRECTORY "/tree/json/config/", "config_base_id");
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_config_set_keyval_string (context_section,
                                     CMAKE_BUILD_DIRECTORY "/tree/json/mold/", "mold_base_id");
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

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "EXIT DisirTestTestPlugin SetupTestCase");
}

void
testing::DisirTestTestPlugin::TearDownTestCase ()
{
    if (instance)
    {
        enum disir_status s = disir_instance_destroy (&instance);
        EXPECT_STATUS (DISIR_STATUS_OK, s);
    }
}

