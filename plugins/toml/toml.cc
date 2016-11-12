#include <disir/fslib/toml.h>

enum disir_status dio_toml_config_entries (struct disir_instance *disir,
                                           void *storage, struct disir_entry **entries)
{

    // Return all CONFIG entires as an array of disir_entry

    // Get the list of available molds entries
    //

}


extern "C" enum disir_status
dio_register_plugin (struct disir_instance *disir, const char *name)
{
    struct disir_plugin plugin;

    plugin.dp_name = (char *) "TOML";
    plugin.dp_description = (char *) "A TOML config, JSON mold, filesystem based plugin";
    plugin.dp_type = (char *) "toml";
    // Storage space unused.
    plugin.dp_storage = NULL;
    plugin.dp_plugin_finished = NULL;

    plugin.dp_config_read = dio_toml_config_read;
    plugin.dp_config_write = dio_toml_config_write;
    plugin.dp_config_entries = dio_toml_config_entries;
    plugin.dp_config_query = dio_toml_config_query;

    // TODO: Use JSON based mold output.
    plugin.dp_mold_read = NULL;
    plugin.dp_mold_write = NULL;
    plugin.dp_mold_entries = NULL;
    plugin.dp_mold_query = NULL;

    return disir_plugin_register (disir, &plugin);
}

