#ifndef _LIBDISIR_DISIR_PRIVATE_H
#define _LIBDISRI_DISIR_PRIVATE_H

#include <disir/disir.h>
#include <disir/io.h>

//! \brief The main libdisir instance structure. All I/O operations requires an instance of it.
struct disir
{
    //! Double-linked list queue of registered input plugins
    struct disir_input      *dio_input_queue;
    //! Double-linked list queue of registered output plugins
    struct disir_output     *dio_output_queue;

    //! Mold of configuration entry for libdisir itself.
    struct disir_mold       *libdisir_mold;

    //! Error message sat on the disir instance.
    //! Set with disir_error_set() and clear with disir_error_clear()
    //! Retrievable through disir_error() and disir_error_copy()
    char                    *disir_error_message;
    //! Bytes allocated/occupied by the disir_error_message.
    int32_t                 disir_error_message_size;
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

// TMP - REWORK
enum disir_status
dio_register_ini (struct disir *disir);


#endif // _LIBDISIR_DISIR_PRIVATE_H

