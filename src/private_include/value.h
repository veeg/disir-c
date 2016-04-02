#ifndef _LIBDISIR_VALUE_H
#define _LIBDISIR_VALUE_H

struct disir_value
{
    enum disir_type dv_type;
    union
    {
        char        *dv_string;
        uint8_t     dv_boolean;
        int64_t     dv_enum;
        double      dv_float;
        int64_t     dv_integer;

    };

    int64_t         dv_size;
};

//! INTERNAL API
enum disir_status
dx_value_set_string (struct disir_value *value, const char *string, int32_t string_size);

//! INTERNAL API
//! \param size filled with string size if not NULL.
enum disir_status
dx_value_get_string (struct disir_value *value, const char **string, int32_t *size);


#endif // _LIBDISIR_VALUE_H

