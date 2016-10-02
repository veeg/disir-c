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
    char                *pi_name;

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


// TODO: Remove dio_ini_config_read and dio_ini_config_write.
//          Replace with a toml/json hybrid instead.

//! \brief Read INI formatted configuration file from id filepath
//!
//! \param[in] disir A libdisir instance to associate with the read operation.
//! \param[in] id An entry identifier for which file to read. Corresponds to a full filepath.
//! \param[in] mold A associated mold with this config read operation. Required parameter.
//! \param[out] config Output parameter populated with the read config object.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
dio_ini_config_read (struct disir_instance *disir, const char *id,
                     struct disir_mold *mold, struct disir_config **config);

//! \brief Write INI formatted configuration file to id filepath
//!
//! \param[in] disir A libdisir instance to associate with the write operation.
//! \param[in] id An entry identifier for which to write the file. Corresponds to a full filepath.
//! \param[in] config The config object to persist to disk at filepath location `id`.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
dio_ini_config_write (struct disir_instance *disir, const char *id, struct disir_config *config);


#endif // _LIBDISIR_DISIR_PRIVATE_H

