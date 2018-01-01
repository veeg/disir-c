#ifndef _LIBDISIR_FSLIB_JSON_H
#define _LIBDISIR_FSLIB_JSON_H

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


//! \brief JSON implementation of config_read
//!
enum disir_status
dio_json_config_read (struct disir_instance *instance,
                      struct disir_register_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config);

//! \brief JSON implementation of config_write
//!
enum disir_status
dio_json_config_write (struct disir_instance *instance,
                       struct disir_register_plugin *plugin, const char *entry_id,
                       struct disir_config *config);

//! \brief JSON implementation of config_remove
//!
enum disir_status
dio_json_config_remove (struct disir_instance *instance,
                        struct disir_register_plugin *plugin, const char *entry_id);

//! \brief JSON implementation of config_write to filedescriptor
//!
enum disir_status
dio_json_config_fd_write (struct disir_instance *instance,
                          struct disir_config *config, FILE *out);

//! \brief JSON implementation of config_read to filedescriptor
//!
enum disir_status
dio_json_config_fd_read (struct disir_instance *instance, FILE *in,
                         struct disir_mold *mold, struct disir_config **config);

//! \brief JSON imlementation of config_entries
//!
enum disir_status
dio_json_config_entries (struct disir_instance *instance,
                         struct disir_register_plugin *plugin,
                         struct disir_entry **entries);

//! \brief JSON imlementation of config_query
//!
enum disir_status
dio_json_config_query (struct disir_instance *instance,
                       struct disir_register_plugin *plugin,
                       const char *entry_id,
                       struct disir_entry **entry);


//! \brief JSON implementation of mold_read
//!
enum disir_status
dio_json_mold_read (struct disir_instance *instance,
                    struct disir_register_plugin *plugin, const char *entry_id,
                    struct disir_mold **mold);

//! \brief JSON implementation of mold_write
//!
enum disir_status
dio_json_mold_write (struct disir_instance *instance,
                     struct disir_register_plugin *plugin, const char *entry_id,
                     struct disir_mold *mold);

//! \brief JSON imlementation of mold_entries
//!
enum disir_status
dio_json_mold_entries (struct disir_instance *instance,
                       struct disir_register_plugin *plugin,
                       struct disir_entry **entries);

//! \brief JSON imlementation of mold_query
//!
enum disir_status
dio_json_mold_query (struct disir_instance *instance,
                     struct disir_register_plugin *plugin,
                     const char *entry_id,
                     struct disir_entry **entry);

//! TODO: docs
enum disir_status
dio_json_serialize_config (struct disir_instance *instance,
                           struct disir_config *config, FILE *output);

//! TODO: docs
enum disir_status
dio_json_unserialize_config (struct disir_instance *instance, FILE *input,
                             struct disir_mold *mold, struct disir_config **config);

//! TODO: docs
enum disir_status
dio_json_serialize_mold (struct disir_instance *instance,
                         struct disir_mold *mold, FILE *output);

//! TODO: docs
enum disir_status
dio_json_unserialize_mold (struct disir_instance *instance,
                           FILE *input, struct disir_mold **mold);

enum disir_status
dio_json_unserialize_mold_filepath (struct disir_instance *instance,
                                    const char *filepath, const char *override_filepath,
                                    struct disir_mold **mold);

#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_FSLIB_JSON_H

