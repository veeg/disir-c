#ifndef _LIBDISIR_PRIVATE_DISIR_H
#define _LIBDISIR_PRIVATE_DISIR_H

#include <disir/disir.h>
#include <disir/plugin.h>

//! Internal plugin structure
struct disir_register_plugin_internal
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
    struct disir_register_plugin pi_plugin;

    struct disir_register_plugin_internal *next, *prev;
};

//! \brief The main libdisir instance structure. All I/O operations requires an instance of it.
struct disir_instance
{
    //! Double-linked list queue loaded plugins
    struct disir_register_plugin_internal    *dio_plugin_queue;

    //! Active configuration based of libdisir_mold
    struct disir_config             *libdisir_config;

    //! Error message sat on the disir instance.
    //! Set with disir_error_set() and clear with disir_error_clear()
    //! Retrievable through disir_error() and disir_error_copy()
    char                            *disir_error_message;
    //! Bytes allocated/occupied by the disir_error_message.
    int32_t                         disir_error_message_size;
};

//! \brief get disir_register_plugin by group id
enum disir_status
dx_retrieve_plugin_by_group (struct disir_instance *instance, const char *group_id,
                             struct disir_register_plugin **plugin);

#endif // _LIBDISIR_PRIVATE_DISIR_H

