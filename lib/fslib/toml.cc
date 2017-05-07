// public
#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/fslib/toml.h>


//! PLUGIN API
enum disir_status
dio_toml_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    return fslib_plugin_config_read (instance, plugin, entry_id, mold,
                                     config, dio_toml_unserialize_config);
}

//! PLUGIN API
enum disir_status
dio_toml_config_write (struct disir_instance *instance, struct disir_plugin *plugin,
                       const char *entry_id, struct disir_config *config)
{
    return fslib_plugin_config_write (instance, plugin, entry_id,
                                      config, dio_toml_serialize_config);
}

//! PLUGIN API
enum disir_status
dio_toml_config_entries (struct disir_instance *instance,
                         struct disir_plugin *plugin,
                         struct disir_entry **entries)
{
    return fslib_config_query_entries (instance, plugin, NULL, entries);
}

// PLUGIN API
enum disir_status
dio_toml_config_query (struct disir_instance *instance, struct disir_plugin *plugin,
                       const char *entry_id, struct disir_entry **entry)
{
    return fslib_plugin_config_query (instance, plugin, entry_id, entry);
}

