#include <disir/disir.h>
#include <disir/fslib/json.h>

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *disir, struct disir_plugin *plugin)
{

    plugin->dp_name = (char *) "JSON";
    plugin->dp_description = (char *) "JSON config, JSON mold";

    plugin->dp_storage = NULL;
    plugin->dp_plugin_finished = NULL;

    plugin->dp_config_entry_type = (char *) "json";
    plugin->dp_config_read = dio_json_config_read;
    plugin->dp_config_write = dio_json_config_write;
    plugin->dp_config_entries = dio_json_config_entries;
    plugin->dp_config_query = dio_json_config_query;

    plugin->dp_mold_entry_type = (char *) "json";
    plugin->dp_mold_read = dio_json_mold_read;
    plugin->dp_mold_write = dio_json_mold_write;
    plugin->dp_mold_entries = dio_json_mold_entries;
    plugin->dp_mold_query = dio_json_mold_query;

    return DISIR_STATUS_OK;
}

