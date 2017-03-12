#include <disir/plugin.h>
#include <disir/fslib/json.h>
#include <disir/test.h>


extern "C" enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_plugin *plugin)
{
    plugin->dp_name = (char *) "JSON Config Test";
    plugin->dp_description = (char *) "JSON config, TEST mold";
    // Storage space unused.
    plugin->dp_storage = NULL;
    plugin->dp_plugin_finished = NULL;

    plugin->dp_config_entry_type = (char *) "json";
    plugin->dp_config_read = dio_json_config_read;
    plugin->dp_config_write = dio_json_config_write;
    plugin->dp_config_entries = dio_json_config_entries;
    plugin->dp_config_query = dio_json_config_query;

    plugin->dp_mold_entry_type = (char *) "json";
    plugin->dp_mold_read = dio_test_mold_read;
    plugin->dp_mold_write = NULL;
    plugin->dp_mold_entries = dio_test_mold_entries;;
    plugin->dp_mold_query = dio_test_mold_query;

    return DISIR_STATUS_OK;
}

