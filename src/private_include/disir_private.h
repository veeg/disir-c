#ifndef _LIBDISIR_DISIR_PRIVATE_H
#define _LIBDISIR_DISIR_PRIVATE_H

#include <disir/disir.h>
#include <disir/io.h>

//! \brief The main libdisir instance structure. All I/O operations requires an instance of it.
struct disir_instance
{
    //! Double-linked list queue of dynamically loaded plugins
    struct disir_plugin     *dio_plugin_queue;

    //! Double-linked list queue of registered input plugins
    struct disir_input      *dio_input_queue;
    //! Double-linked list queue of registered output plugins
    struct disir_output     *dio_output_queue;

    //! Active configuration based of libdisir_mold
    struct disir_config     *libdisir_config;
    //! Mold of configuration entry for libdisir itself.
    struct disir_mold       *libdisir_mold;

    //! Error message sat on the disir instance.
    //! Set with disir_error_set() and clear with disir_error_clear()
    //! Retrievable through disir_error() and disir_error_copy()
    char                    *disir_error_message;
    //! Bytes allocated/occupied by the disir_error_message.
    int32_t                 disir_error_message_size;
};

struct disir_plugin
{
    void                    *pl_dl_handler;
    char                    *pl_filepath;

    struct disir_plugin     *next, *prev;
};

//! \brief The internal input plugin structure.
struct disir_input
{
    //! String identifier of the input type.
    char                            *di_type;
    //! String description of the input type.
    char                            *di_description;

    //! Input structure holds all function callbaks to perform the input for this plugin.
    struct disir_input_plugin       di_input;

    //! linked list pointers
    struct disir_input *next, *prev;
};

//! \brief The internal output plugin structure.
struct disir_output
{
    //! String identifier of the output type.
    char                            *do_type;
    //! String description of the output type.
    char                            *do_description;

    //! Output structure holds all function callbacks to perform the output for this plugin.
    struct disir_output_plugin      do_output;

    struct disir_output *next, *prev;
};

//! \brief Allocate a disir_output structure
struct disir_output * dx_disir_output_create (void);

//! \brief Destroy a previously allocated disir_output structure
enum disir_status dx_disir_output_destroy (struct disir_output **output);

//! \brief Allocate a disir_input structure
struct disir_input * dx_disir_input_create (void);

//! \brief Destroy a previously allocated disir_input structure
enum disir_status dx_disir_input_destroy (struct disir_input **output);

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

