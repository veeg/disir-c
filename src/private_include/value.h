#ifndef _LIBDISIR_VALUE_H
#define _LIBDISIR_VALUE_H

struct disir_value
{
    enum disir_value_type dv_type;
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

//! \brief Return a string represetation of the passed value type enum
//!
//! \param[in] type Enumeration that describes some value type
//!
//! \return string representation of the input 'type. UNKNOWN is returned
//!     if the input type is out of bounds.
//!
const char * dx_value_type_string (enum disir_value_type type);

//! \brief Sanitize the input enum to a valid enum range
//!
//! Out-of-bounds enumeration values are converted to DISIR_VALUE_TYPE_UNKNOWN.
//!
//! \param type Enumeration that describes some value type to be sanitized.
//!
//! \return Sanitized enumeration that is guaranteed to be within bounds.
//!
enum disir_value_type dx_value_type_sanify (enum disir_value_type type);


//! \brief Set the input 'value' with the contents of the input 'string'
//!
//! Allocate and copy the input 'string' into the 'value' structure.
//! Only 'string_size' bytes are copied and then an additional \0 terminator
//! is appended to the stored string. Passing in a NULL string pointer results in
//! the value to be emptied and set to zero length.
//!
//! \param value Value object to store a value type.
//! \param string Input string which shall be stored in the 'value' object.
//! \param string_size Length of the input string. Only 'string_size' number of bytes
//!     are copied into the 'value' object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if 'value' are NULL pointer
//!     or that the value type of 'value' is not DISIR_VALUE_TYPE_STRING.
//! \return DISIR_STATUS_NO_MEMORY if an allocation failed for the value object.
//! \return DISIR_STATUS_OK if 'value' is successfully populated with the contents of 'string'
//!
enum disir_status
dx_value_set_string (struct disir_value *value, const char *string, int32_t string_size);

//! \brief Reterieve the string type stored in value
//!
//! \param[in] value Value object to retrieve the string value from.
//! \param[out] string Pointer is to redirect to the stored string in 'value'
//! \param[out] size Populated with the size of the output string size if not NULL.
//!
//! \return DISIR_STATUS_OK if the 'string' input pointer is pointed to the stored string.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input 'value' or 'string' are NULL pointers,
//!     or the value type of 'value' is not DISIR_VALUE_TYPE_STRING
//!
enum disir_status
dx_value_get_string (struct disir_value *value, const char **string, int32_t *size);


//! \brief Set the input 'value' with the contents of the input 'integer'
//!
//! \param value Value object to store a value type.
//! \param integer Input integer value which shall be stored in the 'value' object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if 'value' are NULL pointer
//!     or that the value type of 'value' is not DISIR_VALUE_TYPE_INTEGER.
//! \return DISIR_STATUS_OK if 'value' is successfully populated with the contents of 'integer'
//!
enum disir_status
dx_value_set_integer (struct disir_value *value, int64_t integer);

//! \brief Reterieve the integer type stored in value
//!
//! \param[in] value Value object to retrieve the integer value from.
//! \param[out] integer Populated with the integer value of 'value' object.
//!
//! \return DISIR_STATUS_OK if the 'integer' input pointer is populated with stored value.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input 'value' or 'integer' are NULL pointers,
//!     or the value type of 'value' is not DISIR_VALUE_TYPE_INTEGER.
//!
enum disir_status
dx_value_get_integer (struct disir_value *value, int64_t *integer);

//! \brief Set the input 'value' with the contents of the input 'output_double'
//!
//! \param value Value object to store a value type.
//! \param input_double Input double value which shall be stored in the 'value' object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if 'value' are NULL pointer
//!     or that the value type of 'value' is not DISIR_VALUE_TYPE_FLOAT.
//! \return DISIR_STATUS_OK if 'value' is successfully populated
//!     with the contents of 'input_double'
//!
enum disir_status
dx_value_set_float (struct disir_value *value, double input_double);


//! \brief Reterieve the float type stored in value
//!
//! \param[in] value Value object to retrieve the float value from.
//! \param[out] output_double Populated with the float  value of 'value' object.
//!
//! \return DISIR_STATUS_OK if the 'output_double' input pointer is populated with stored value
//! \return DISIR_STATUS_INVALID_ARGUMENT if input 'value' or 'output_double' are NULL pointers,
//!     or the value type of 'value' is not DISIR_VALUE_TYPE_FLOAT.
//!
enum disir_status
dx_value_get_float (struct disir_value *value, double *output_double);


#endif // _LIBDISIR_VALUE_H

