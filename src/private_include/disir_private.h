#ifndef _LIBDISIR_DISIR_PRIVATE_H
#define _LIBDISRI_DISIR_PRIVATE_H

struct disir
{
    struct disir_input      *dio_input_queue;
    struct disir_output     *dio_output_queue;

    struct disir_schema     *internal_schema;
};

// TMP - REWORK
enum disir_status
dio_register_print (struct disir *disir);

// TMP - REWORK
enum disir_status
dio_register_ini (struct disir *disir);


#endif // _LIBDISIR_DISIR_PRIVATE_H

