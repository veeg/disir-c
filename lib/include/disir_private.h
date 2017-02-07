#ifndef _LIBDISIR_DISIR_PRIVATE_H
#define _LIBDISIR_DISIR_PRIVATE_H

#include <disir/disir.h>
#include <disir/io.h>
#include <disir/plugin.h>

//! Internal plugin structure
struct disir_plugin_internal
{
    //! Allocated dl_open handler (if registerd dynamically)
    void                *pi_dl_handler;
    //! Filepath to shared library (if registered dynamically)
    char                *pi_filepath;

    //! Internal name given to this plugin when it registered.
    char                *pi_io_id;

    //! Group this plugin is registerd to.
    char                *pi_group_id;

    //! Copy of plugin parameters this plugin registered itself with.
    struct disir_plugin pi_plugin;

    struct disir_plugin_internal *next, *prev;
};

//! \brief The main libdisir instance structure. All I/O operations requires an instance of it.
struct disir_instance
{
    //! Double-linked list queue loaded plugins
    struct disir_plugin_internal    *dio_plugin_queue;

    //! Active configuration based of libdisir_mold
    struct disir_config             *libdisir_config;
    //! Mold of configuration entry for libdisir itself.
    struct disir_mold               *libdisir_mold;

    //! Error message sat on the disir instance.
    //! Set with disir_error_set() and clear with disir_error_clear()
    //! Retrievable through disir_error() and disir_error_copy()
    char                            *disir_error_message;
    //! Bytes allocated/occupied by the disir_error_message.
    int32_t                         disir_error_message_size;
};

//! \brief Output the libdisir config entry to disk.
//!
//! \param[in] instance Disir library instance associated with this operation.
//! \param[in] config Valid configuration object based of libdisir_mold
//! \param[in] filepath Path to disk location to write the INI-formatted config file.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status disir_libdisir_config_to_disk (struct disir_instance *instance,
                                                 struct disir_config *config,
                                                 const char *filepath);

//! \brief Read the libdisri config entry from disk.
//!
//! \param[in] instance Disir library instance associated with this operation.
//! \param[in] filepath Full filepath to the location on disk to locate the configuration file.
//! \param[in] mold Mold instance for libdisir.
//! \param[out] config The config structure to populate with the read configuration file.
//
//! \return DISIR_STATUS_INVALID_ARGUMENT if config_filepath cannot be opened.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_libdisir_config_from_disk (struct disir_instance *instance,
                                 const char *filepath,
                                 struct disir_mold *mold,
                                 struct disir_config **config);

#endif // _LIBDISIR_DISIR_PRIVATE_H

