#include <disir/plugin.h>
#include <disir/test.h>

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *instance, const char *name)
{
    struct disir_plugin plugin;

    plugin.dp_name = (char *) "test";
    plugin.dp_description = (char *) "A collection of various molds to enumerate functionality in libdisir";
    plugin.dp_type = (char *) "test";
    // Storage space unused.
    plugin.dp_storage = NULL;
    plugin.dp_plugin_finished = NULL;

    plugin.dp_config_read = dio_test_config_read;
    plugin.dp_config_write = NULL;
    plugin.dp_config_entries = dio_test_config_entries;
    plugin.dp_config_query = dio_test_config_query;

    plugin.dp_mold_read = dio_test_mold_read;
    plugin.dp_mold_write = NULL;
    plugin.dp_mold_entries = dio_test_mold_entries;
    plugin.dp_mold_query = dio_test_mold_query;

    return disir_plugin_register (instance, &plugin);
}

