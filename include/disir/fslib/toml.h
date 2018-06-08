#ifndef _LIBDISIR_FSLIB_TOML_H
#define _LIBDISIR_FSLIB_TOML_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


#include <disir/disir.h>
#include <disir/plugin.h>
#include <stdio.h>

//! PLEASE NOTE:
//! This file contains public libdisir methods and structures
//! that is only indended for use by plugins.
//! Any usage of these methods outside plugins may cause inconsistencies
//! in the deployed configurations in a filesystem.


//! \brief TOML implementation of config_read
//!
DISIR_EXPORT
enum disir_status
dio_toml_config_read (struct disir_instance *instance,
                      struct disir_register_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config);

//! \brief TOML implementation of config_write
//!
DISIR_EXPORT
enum disir_status
dio_toml_config_write (struct disir_instance *instance,
                       struct disir_register_plugin *plugin, const char *entry_id,
                       struct disir_config *config);

//! \brief TOML imlementation of config_entries
//!
DISIR_EXPORT
enum disir_status
dio_toml_config_entries (struct disir_instance *instance,
                         struct disir_register_plugin *plugin,
                         struct disir_entry **entries);

//! \brief TOML imlementation of config_query
//!
DISIR_EXPORT
enum disir_status
dio_toml_config_query (struct disir_instance *instance,
                       struct disir_register_plugin *plugin,
                       const char *entry_id,
                       struct disir_entry **entry);

//! TODO: docs
DISIR_EXPORT
enum disir_status
dio_toml_serialize_config (struct disir_instance *instance,
                           struct disir_config *config, FILE *output);

//! TODO: docs
DISIR_EXPORT
enum disir_status
dio_toml_unserialize_config (struct disir_instance *instance, FILE *input,
                             struct disir_mold *mold, struct disir_config **config);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_FSLIB_TOML_H

