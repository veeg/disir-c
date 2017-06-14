
// PUBLIC API
#include <disir/disir.h>
#include <disir/plugin.h>

#include "test_helper.h"


//
// This class tests the public API functions:
//  disir_plugin_registered
//  disir_plugin_finished
//
class PluginInfoTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp();

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();
        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status = DISIR_STATUS_OK;
};

TEST_F (PluginInfoTest, registered_invalid_arguments)
{
    struct disir_plugin *plugins;
    ASSERT_NO_SETUP_FAILURE();

    status = disir_plugin_registered (NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_plugin_registered (instance, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = disir_plugin_registered (NULL, &plugins);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (PluginInfoTest, registered)
{
    struct disir_plugin *plugins;
    struct disir_plugin *current;
    struct disir_plugin *last;
    ASSERT_NO_SETUP_FAILURE();

    status = disir_plugin_registered (instance, &plugins);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    last = plugins;

    current = last;
    ASSERT_TRUE (current != NULL);
    ASSERT_STREQ ("test", current->pl_group_id);

    current = current->next;
    ASSERT_TRUE (current == NULL);

    disir_plugin_finished (&last);
}
