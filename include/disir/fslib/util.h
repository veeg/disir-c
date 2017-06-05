#ifndef _LIBDISIR_FSLIB_UTIL_H
#define _LIBDISIR_FSLIB_UTIL_H

#include <disir/disir.h>
#include <disir/plugin.h>

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

//! Function signature for standard filesystem serializable config format
typedef enum disir_status (*dio_serialize_config) (struct disir_instance *,
                                                   struct disir_config *,
                                                   FILE *);

//! Function signature for standard filesystem unserializable config format
typedef enum disir_status (*dio_unserialize_config) (struct disir_instance *,
                                                     FILE *,
                                                     struct disir_mold *,
                                                     struct disir_config **);

//! Function signature for standard filesystem serializable mold format
typedef enum disir_status (*dio_serialize_mold) (struct disir_instance *,
                                                 struct disir_mold *,
                                                 FILE *);

//! Function signature for standard filesystem unserializable mold format
typedef enum disir_status (*dio_unserialize_mold) (struct disir_instance *,
                                                   FILE *,
                                                   struct disir_mold **);


//! Create the input path recursively
//! Similar to shall command mkdir -p
//!
//! \return DISIR_STATUS_PERMISSION_ERROR on EACCES
//! \return DISIR_STATUS_FS_ERROR on any other error condition
//! \return DISIR_STATUS_OK on success (entire path may already exist as directories)
//!
enum disir_status
fslib_mkdir_p (struct disir_instance *instance, const char *path);


//! Create namespace entry of input name
//!
//! return empty string if no such namespace entry can be created
//!
const char *
fslib_namespace_entry (const char *name, char *namespace_entry);

//! \brief Resolve entry_id to plugin filepath for config.
//!
//! \param[out] filepath Populate the output buffer with the complete, resolved filepath.
//!     Buffer is required to be PATH_MAX sized.
//!
//! \return DISIR_STATUS_INSUFFICIENT_RESOURCES if the resolved path is too large.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
fslib_config_resolve_filepath (struct disir_instance *instance, struct disir_register_plugin *plugin,
                               const char *entry_id, char *filepath);

//! \brief Resolve entry_id to plugin filepath for mold.
//!
//! \param[out] filepath Populate the output buffer with the complete, resolved filepath.
//!     Buffer is required to be PATH_MAX sized.
//!
//! \return DISIR_STATUS_INSUFFICIENT_RESOURCES if the resolved path is too large.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
fslib_mold_resolve_filepath (struct disir_instance *instance, struct disir_register_plugin *plugin,
                             const char *entry_id, char *filepath);

//! \brief Resolve the complete mold entry for entry_id.
//!
//! \param[out] filepath Populate the output buffer with the filepath to the mold
//!     active for entry_id.
//! \param[out] statbuf Stat results of filepath.
//! \param[out] namespace_entry Boolean result indicating if filepath is a namespace entry
//!     for entry_id.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
fslib_mold_resolve_entry_id (struct disir_instance *instance, struct disir_register_plugin *plugin,
                             const char *entry_id, char *filepath,
                             struct stat *statbuf, int *namespace_entry);

//! \brief Stat the filepath and return appropriate error (with message set)
//!
//! \return DISIR_STATUS_FS_ERROR if filepath contains a non-directory in path.
//! \return DISIR_STATUS_PERMISSION_ERROR if user has insufficient permission to stat filepath.
//! \return DISIR_STATUS_NOT_EXIST if filepath does not exist.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
fslib_stat_filepath (struct disir_instance *instance,
                     const char *filepath, struct stat *statbuf);

//! \brief Recursively query basedir for matching plugin entries.
//!
//! \return DISIR_STATUS_OK regardless of query operation
//!
enum disir_status
fslib_config_query_entries (struct disir_instance *instance, struct disir_register_plugin *plugin,
                            const char *basedir, struct disir_entry **entries);

//! \brief Generic recursive query implementation of mold_entries
enum disir_status
fslib_mold_query_entries (struct disir_instance *instance, struct disir_register_plugin *plugin,
                          const char *basedir, struct disir_entry **entries);

//! \brief Generic filesystem based implementation of config_read
enum disir_status
fslib_plugin_config_read (struct disir_instance *instance,
                          struct disir_register_plugin *plugin, const char *entry_id,
                          struct disir_mold *mold, struct disir_config **config,
                          dio_unserialize_config func_unserialize);

//! \brief Generic filesystem based implementation of mold_read
enum disir_status
fslib_plugin_mold_read (struct disir_instance *instance,
                        struct disir_register_plugin *plugin, const char *entry_id,
                        struct disir_mold **mold,
                        dio_unserialize_mold func_unserialize);

//! \brief Generic filesystem based implementation of config_write
enum disir_status
fslib_plugin_config_write (struct disir_instance *instance, struct disir_register_plugin *plugin,
                           const char *entry_id, struct disir_config *config,
                           dio_serialize_config func_serialize);

//! \brief Generic filesystem based implementation of mold_write
enum disir_status
fslib_plugin_mold_write (struct disir_instance *instance, struct disir_register_plugin *plugin,
                         const char *entry_id, struct disir_mold *mold,
                         dio_serialize_mold func_serialize);

//! \brief Generic filesystem based implementation of config_query
enum disir_status
fslib_plugin_config_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                           const char *entry_id, struct disir_entry **entry);

//! \brief Generic filesystem based implementation of mold_query
enum disir_status
fslib_plugin_mold_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                         const char *entry_id, struct disir_entry **entry);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_FSLIB_UTIL_H

