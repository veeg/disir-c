#include <disir/plugin.h>
#include <disir/fslib/toml.h>

#define RM_CONST(t, exp) (t*)((char*)NULL + ((const char*)(exp) - (char*)NULL))

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin);

enum disir_status
dio_register_plugin (struct disir_instance *instance, struct disir_register_plugin *plugin)
{
    (void) &instance;

    plugin->dp_name = RM_CONST (char, "TOML");
    plugin->dp_description = RM_CONST (char, "A TOML config, JSON mold, filesystem based plugin");
    // Storage space unused.
    plugin->dp_storage = NULL;
    plugin->dp_plugin_finished = NULL;

    plugin->dp_config_entry_type = RM_CONST (char, "toml");
    plugin->dp_config_read = dio_toml_config_read;
    plugin->dp_config_write = dio_toml_config_write;
    plugin->dp_config_remove = NULL;
    plugin->dp_config_entries = dio_toml_config_entries;
    plugin->dp_config_query = dio_toml_config_query;

    // TODO: Use JSON based mold output.
    plugin->dp_mold_entry_type = RM_CONST (char, "json");
    plugin->dp_mold_read = NULL;
    plugin->dp_mold_write = NULL;
    plugin->dp_mold_entries = NULL;
    plugin->dp_mold_query = NULL;

    return DISIR_STATUS_OK;
}

