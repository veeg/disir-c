#ifndef _LIBDISIR_PLUGIN_H
#define _LIBDISIR_PLUGIN_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus

#include <disir/disir.h>
#include <stdlib.h>

// Forward declare structure for typedef's below
struct disir_plugin;

//! \brief Function signature for plugin to implement to register with Disir instance.
typedef enum disir_status (*plugin_register) (struct disir_instance *instance, const char *name);

//! \brief Function signature for plugin to implement to cleanup loaded plugin state.
typedef enum disir_status (*plugin_finished) (struct disir_instance *instance,
                                              struct disir_plugin *plugin);


//! \brief Function signature for plugin to implement reading config object.
//!
//! It is the callers responsibility to invoke disir_mold_finished() if he
//! allocates one and it is not provided by the callee.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the config entry to read.
//! \param[in] mold Optional mold to validate config against. If not provided, function
//!     is responsible to locate and create the mold associated with id.
//! \param[out] config The fully populated config object
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_read) (struct disir_instance *instance,
                                          struct disir_plugin *plugin,
                                          const char *entry_id,
                                          struct disir_mold *mold,
                                          struct disir_config **config);

//! \brief Function signature for plugin to implement writing config object.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the config entry to write.
//! \param[in] config The config object to persist to 'id' locatiton.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_write) (struct disir_instance *instance,
                                           struct disir_plugin *plugin,
                                           const char *entry_id,
                                           struct disir_config *config);

//! \brief Function signature for plugin to implement retrieval of all available config entries.
//!
//! Caller is responsible to invoke disir_entry_finished () on (every)
//! entries returned in the double-linked list of disir_entry' structures.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[out] entries Double-linked list of heap-allocated entries. Only populated on success.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_entries) (struct disir_instance *instance,
                                             struct disir_plugin *plugin,
                                             struct disir_entry **entries);

//! \brief Function signature for plugin to implement querying of config entry's existence.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the config entry to query for.
//!
//! \return DISIR_STATUS_NOT_EXIST if plugin does not contain `entry_id` config.
//! \return DISIR_STATUS_EXISTS if `entry_id` config is provided by plugin.
//!
typedef enum disir_status (*config_query) (struct disir_instance *instance,
                                           struct disir_plugin *plugin,
                                           const char *entry_id);

//! \brief Function signature for plugin to implement reading mold object.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the mold entry to read.
//! \param[out] mold The fully populated mold object.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*mold_read) (struct disir_instance *instance,
                                        struct disir_plugin *plugin,
                                        const char *entry_id,
                                        struct disir_mold **mold);

//! \brief Function signature for plugin to implement writing mold object.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the mold entry to write.
//! \param[in] config The config object to persist to 'id' locatiton.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*mold_write) (struct disir_instance *instance,
                                         struct disir_plugin *plugin,
                                         const char *entry_id,
                                         struct disir_mold *mold);

//! \brief Function signature for plugin to implement retrieval of all available mold entries.
//!
//! Caller is responsible to invoke disir_entry_finished () on (every)
//! entries returned in the double-linked list of disir_entry' structures.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[out] entries Double-linked list of heap-allocated entries. Only populated on success.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*mold_entries) (struct disir_instance *instance,
                                           struct disir_plugin *plugin,
                                           struct disir_entry **entries);

//! \brief Function signature for plugin to implement querying of mold entry's existence.
//!
//! \param[in] instance Library instance associated with this I/O operation.
//! \param[in] plugin The plugin instance this operation is associated with.
//! \param[in] entry_id String identifier for the mold entry to query for.
//!
//! \return DISIR_STATUS_NOT_EXIST if plugin does not contain `entry_id` mold.
//! \return DISIR_STATUS_EXISTS if `entry_id` mold is provided by plugin.
//!
typedef enum disir_status (*mold_query) (struct disir_instance *instance,
                                         struct disir_plugin *plugin,
                                         const char *entry_id);


//! Disir Plugin - Plugins populate this structure to register itself with a Disir instance.
struct disir_plugin
{
    //! Identify the version of libdisir this plugin is atleast compatible with.
    uint32_t        dp_major_version;
    uint32_t        dp_minor_version;

    //! Internal name identify of this plugin
    char            *dp_name;

    //! String description of this plugin.
    char            *dp_description;

    //! Short string description of what type this plugin is.
    //! e.g., TOML, JSON, sqlite3
    char            *dp_type;

    //! Pointer to plugin-specific storage space. Allocated and free'd by the plugin.
    void            *dp_storage;

    plugin_finished dp_plugin_finished;

    config_read     dp_config_read;
    config_write    dp_config_write;
    config_entries  dp_config_entries;
    config_query    dp_config_query;
    mold_read       dp_mold_read;
    mold_write      dp_mold_write;
    mold_entries    dp_mold_entries;
    mold_query      dp_mold_query;
};

//! \brief Register plugin with the libdisir instance
//!
//! Plugin must provide a disir_plugin structure that is populated with all
//! information and callback provided by plugin. The `disir` instance will deep-copy the settings
//! provided, so it is the callers responsibility to free the input structure, and any memory
//! dynamically allocated within it.
//!
//! Once the plugin is de-registered with `disir`, the plugin->dp_plugin_finished callback
//! will be invoked, allowing the plugin to cleanup and de-allocate storage memory.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input parameters are NULL.
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_plugin_register (struct disir_instance *instance, struct disir_plugin *plugin);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_PLUGIN_H

