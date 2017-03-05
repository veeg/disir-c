#include <disir/disir.h>
#include <disir/fslib/json.h>


//! FSLIB API
enum disir_status
dio_json_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_config_write (struct disir_instance *instance,
                       struct disir_plugin *plugin, const char *entry_id,
                       struct disir_config *config)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_config_entries (struct disir_instance *instance,
                         struct disir_plugin *plugin,
                         struct disir_entry **entries)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_config_query (struct disir_instance *instance,
                       struct disir_plugin *plugin,
                       const char *entry_id,
                       struct disir_entry **entry)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_mold_read (struct disir_instance *instance,
                    struct disir_plugin *plugin, const char *entry_id,
                    struct disir_mold **mold)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_mold_write (struct disir_instance *instance,
                     struct disir_plugin *plugin, const char *entry_id,
                     struct disir_mold *mold)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_mold_entries (struct disir_instance *instance,
                       struct disir_plugin *plugin,
                       struct disir_entry **entries)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! FSLIB API
enum disir_status
dio_json_mold_query (struct disir_instance *instance,
                     struct disir_plugin *plugin,
                     const char *entry_id,
                     struct disir_entry **entry)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

