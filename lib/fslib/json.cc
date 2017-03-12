// public
#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/fslib/json.h>


//! PLUGIN API
enum disir_status
dio_json_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    return fslib_plugin_config_read (instance, plugin, entry_id, mold,
                                     config, dio_json_unserialize_config);
}

//! PLUGIN API
enum disir_status
dio_json_config_write (struct disir_instance *instance,
                       struct disir_plugin *plugin, const char *entry_id,
                       struct disir_config *config)
{
    return fslib_plugin_config_write (instance, plugin, entry_id,
                                      config, dio_json_serialize_config);
}

//! PLUGIN API
enum disir_status
dio_json_config_entries (struct disir_instance *instance,
                         struct disir_plugin *plugin,
                         struct disir_entry **entries)
{
    return fslib_query_entries (instance, plugin, NULL, entries);
}

//! PLUGIN API
enum disir_status
dio_json_config_query (struct disir_instance *instance,
                       struct disir_plugin *plugin,
                       const char *entry_id,
                       struct disir_entry **entry)
{
    return fslib_plugin_config_query (instance, plugin, entry_id, entry);
}

//! PLUGIN API
enum disir_status
dio_json_mold_read (struct disir_instance *instance,
                    struct disir_plugin *plugin, const char *entry_id,
                    struct disir_mold **mold)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PLUGIN API
enum disir_status
dio_json_mold_write (struct disir_instance *instance,
                     struct disir_plugin *plugin, const char *entry_id,
                     struct disir_mold *mold)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PLUGIN API
enum disir_status
dio_json_mold_entries (struct disir_instance *instance,
                       struct disir_plugin *plugin,
                       struct disir_entry **entries)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PLUGIN API
enum disir_status
dio_json_mold_query (struct disir_instance *instance,
                     struct disir_plugin *plugin,
                     const char *entry_id,
                     struct disir_entry **entry)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

