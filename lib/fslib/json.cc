// public
#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/fslib/json.h>


//! PLUGIN API
enum disir_status
dio_json_config_read (struct disir_instance *instance,
                      struct disir_register_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    return fslib_plugin_config_read (instance, plugin, entry_id, mold,
                                     config, dio_json_unserialize_config);
}

//! PLUGIN API
enum disir_status
dio_json_config_write (struct disir_instance *instance,
                       struct disir_register_plugin *plugin, const char *entry_id,
                       struct disir_config *config)
{
    return fslib_plugin_config_write (instance, plugin, entry_id,
                                      config, dio_json_serialize_config);
}

//! PLUGIN API
enum disir_status
dio_json_config_remove (struct disir_instance *instance,
                        struct disir_register_plugin *plugin, const char *entry_id)
{
    return fslib_plugin_config_remove (instance, plugin, entry_id);
}

//! PLUGIN API
enum disir_status
dio_json_config_fd_write (struct disir_instance *instance,
                          struct disir_config *config, FILE *out)
{
    return dio_json_serialize_config (instance, config, out);
}

//! PLUGIN API
enum disir_status
dio_json_config_fd_read (struct disir_instance *instance, FILE *in,
                         struct disir_mold *mold, struct disir_config **config)
{
    return dio_json_unserialize_config (instance, in, mold, config);
}

//! PLUGIN API
enum disir_status
dio_json_config_entries (struct disir_instance *instance,
                         struct disir_register_plugin *plugin,
                         struct disir_entry **entries)
{
    return fslib_config_query_entries (instance, plugin, NULL, entries);
}

//! PLUGIN API
enum disir_status
dio_json_config_query (struct disir_instance *instance,
                       struct disir_register_plugin *plugin,
                       const char *entry_id,
                       struct disir_entry **entry)
{
    return fslib_plugin_config_query (instance, plugin, entry_id, entry);
}

//! PLUGIN API
enum disir_status
dio_json_mold_read (struct disir_instance *instance,
                    struct disir_register_plugin *plugin, const char *entry_id,
                    struct disir_mold **mold)
{
    enum disir_status status;
    char mold_filepath[4096];
    char oe_filepath[4096];
    char *oe_ref = oe_filepath;
    struct stat statbuf;
    int namespace_entry;

    status = fslib_mold_resolve_entry_id (instance, plugin, entry_id,
                                          mold_filepath, oe_filepath, &statbuf, &namespace_entry);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    if (*oe_filepath == '\0')
    {
        oe_ref = NULL;
    }

    status = dio_json_unserialize_mold_filepath (instance, mold_filepath, oe_ref, mold);

    return status;
}

//! PLUGIN API
enum disir_status
dio_json_mold_write (struct disir_instance *instance,
                     struct disir_register_plugin *plugin, const char *entry_id,
                     struct disir_mold *mold)
{
    return fslib_plugin_mold_write (instance, plugin, entry_id,
                                    mold, dio_json_serialize_mold);
}

//! PLUGIN API
enum disir_status
dio_json_mold_entries (struct disir_instance *instance,
                       struct disir_register_plugin *plugin,
                       struct disir_entry **entries)
{
    return fslib_mold_query_entries (instance, plugin, NULL, entries);
}

//! PLUGIN API
enum disir_status
dio_json_mold_query (struct disir_instance *instance,
                     struct disir_register_plugin *plugin,
                     const char *entry_id,
                     struct disir_entry **entry)
{
    return fslib_plugin_mold_query (instance, plugin, entry_id, entry);
}

