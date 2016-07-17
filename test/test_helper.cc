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
    DisirLogCurrentTest ("EXITING TEST SETUP");
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

    enum disir_status    s;
    struct disir_context *context_config;
    struct disir_context *context_keyval;

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "ENTER DisirTestTestPlugin SetupTestCase");

    try
    {
    // Generate config for libdisir
    s = disir_libdisir_mold (&libdisir_mold);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    _log_disir_level (DISIR_LOG_LEVEL_TEST, "DisirTestTestPlugin allocated libdisir mold");

    s = dc_config_begin (libdisir_mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    s = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s = dc_set_name (context_keyval, "plugin_filepath", strlen ("plugin_filepath"));
    ASSERT_STATUS (DISIR_STATUS_OK, s);

    char path[2048];
    strcpy (path, CMAKE_BUILD_DIRECTORY);
    strcat (path, "/plugin_test/dplugin_test.so");
    //std::stringstream ss;
    //ss << CMAKE_BUILD_DIRECTORY << "/plugin_test/dplugin_test.so";
    //s = dc_set_value_string (context_keyval, ss.str().c_str(), ss.str().size());
    s = dc_set_value_string (context_keyval, path, strlen (path));
    ASSERT_STATUS (DISIR_STATUS_OK, s);
    s  = dc_finalize (&context_keyval);
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

