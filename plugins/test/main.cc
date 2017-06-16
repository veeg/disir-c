#include <disir/plugin.h>
#include <disir/test.h>

#define RM_CONST(t, exp) (t*)((char*)NULL + ((const char*)(exp) - (char*)NULL))

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin);

enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin)
{
    (void) &instance;

    plugin->dp_name = RM_CONST (char, "test");
    plugin->dp_description = RM_CONST (char, "A collection of various molds to enumerate functionality in libdisir");
    // Storage space unused.
    plugin->dp_storage = NULL;
    plugin->dp_plugin_finished = NULL;

    plugin->dp_config_entry_type = RM_CONST (char, "test");
    plugin->dp_config_read = dio_test_config_read;
    plugin->dp_config_write = NULL;
    plugin->dp_config_entries = dio_test_config_entries;
    plugin->dp_config_query = dio_test_config_query;

    plugin->dp_mold_entry_type = RM_CONST (char, "test");
    plugin->dp_mold_read = dio_test_mold_read;
    plugin->dp_mold_write = NULL;
    plugin->dp_mold_entries = dio_test_mold_entries;
    plugin->dp_mold_query = dio_test_mold_query;

    return DISIR_STATUS_OK;
}

