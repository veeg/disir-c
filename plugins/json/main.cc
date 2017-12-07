#include <disir/disir.h>
#include <disir/fslib/json.h>

#define RM_CONST(t, exp) (t*)((char*)NULL + ((const char*)(exp) - (char*)NULL))

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin);

enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin)
{
    (void) &instance;

    plugin->dp_name = RM_CONST (char, "JSON");
    plugin->dp_description = RM_CONST (char, "JSON config, JSON mold");

    plugin->dp_storage = NULL;
    plugin->dp_plugin_finished = NULL;

    plugin->dp_config_entry_type = RM_CONST (char, "json");
    plugin->dp_config_read = dio_json_config_read;
    plugin->dp_config_write = dio_json_config_write;
    plugin->dp_config_remove = dio_json_config_remove;
    plugin->dp_config_fd_write = dio_json_config_fd_write;
    plugin->dp_config_fd_read = dio_json_config_fd_read;
    plugin->dp_config_entries = dio_json_config_entries;
    plugin->dp_config_query = dio_json_config_query;

    plugin->dp_mold_entry_type = RM_CONST (char, "json");
    plugin->dp_mold_read = dio_json_mold_read;
    plugin->dp_mold_write = dio_json_mold_write;
    plugin->dp_mold_entries = dio_json_mold_entries;
    plugin->dp_mold_query = dio_json_mold_query;

    return DISIR_STATUS_OK;
}

