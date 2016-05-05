#ifndef _LIBDISIR_DISIR_PRIVATE_H
#define _LIBDISRI_DISIR_PRIVATE_H

#include <disir/disir.h>
#include <disir/io.h>

struct disir
{
    struct disir_input      *dio_input_queue;
    struct disir_output     *dio_output_queue;

    struct disir_mold     *internal_mold;
};

struct disir_input
{
    //! String identifier of the input type.
    char *type;
    //! String description of the input type.
    char *description;

    //! Read a config object
    config_read config;

    //! Read a mold object
    mold_read mold;

    struct disir_input *next, *prev;
};

struct disir_output
{
    //! String identifier of the output type.
    char *type;
    //! String description of the output type.
    char *description;

    //! Write a config object
    config_write config;

    //! Write a mold object
    mold_write mold;

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
dio_register_print (struct disir *disir);

// TMP - REWORK
enum disir_status
dio_register_ini (struct disir *disir);


#endif // _LIBDISIR_DISIR_PRIVATE_H

