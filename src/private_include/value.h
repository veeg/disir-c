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

//! \brief Populate output buffer with a string representation of the value structure
//!
//! Convert the value held by the input value to a string representation that is
//! populated in the output buffer. If the output_buffer_size is of insufficient size,
//! output_size will be equal or greater than output_buffer_size.
//! The output buffer will be populated with maximum output_buffer_size - 1 bytes of
//! stringified output data, where the last byte is always used to NULL terminate.
//! output_size does not include the NULL terminator.
//!
//! No input validation is performed.
//!
//! \param[in] value Structure that hold a value of any type to output.
//! \param[in] output_buffer_size Size of the output buffer
//! \param[in] output Buffer to output string value to.
//! \param[out] output_size Number of bytes value populates buffer with.
//!
//! \return DISIR_STATUS_OK
//!
enum disir_status
dx_value_stringify (struct disir_value *value, int32_t output_buffer_size,
                    char *output, int32_t *output_size);

//! \brief Compare two value structures if they represent the same value
//!
//! Compare two value structures of equal type if the value they hold are of equal type.
//!
//! \return INT_MIN if their type differ
//! \return < 0 if v2 is greater than v1
//! \return > 0 if v1 is greater than v2
//! \return 0 if v1 and v2 are equal
//!
int dx_value_compare (struct disir_value *v1, struct disir_value *v2);

//! \brief Copy source value into destination value
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if types for source and destination differ.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dx_value_copy (struct disir_value *destination, struct disir_value *source);


//! \brief Set the input 'value' with the contents of the input 'input'
//!
//! Allocate and copy the input 'input' into the 'value' structure.
//! Only 'size' bytes are copied and then an additional \0 terminator
//! is appended to the stored string. Passing in a NULL string pointer results in
//! the value to be emptied and set to zero length.
//!
//! \param value Value object to store a value type.
//! \param input Input string which shall be stored in the 'value' object.
//! \param size Length of the input string. Only 'size' number of bytes
//!     are copied into the 'value' object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if 'value' are NULL pointer
//!     or that the value type of 'value' is not DISIR_VALUE_TYPE_STRING.
//! \return DISIR_STATUS_NO_MEMORY if an allocation failed for the value object.
//! \return DISIR_STATUS_OK if 'value' is successfully populated with the contents of 'input'
//!
enum disir_status
dx_value_set_string (struct disir_value *value, const char *input, int32_t size);

//! \brief Reterieve the string type stored in value
//!
//! \param[in] value Value object to retrieve the string value from.
//! \param[out] output Pointer is to redirect to the stored string in 'value'
//! \param[out] size Populated with the size of the output string size if not NULL.
//!
//! \return DISIR_STATUS_OK if the 'output' input pointer is pointed to the stored string.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input 'value' or 'output' are NULL pointers,
//!     or the value type of 'value' is not DISIR_VALUE_TYPE_STRING
//!
enum disir_status
dx_value_get_string (struct disir_value *value, const char **output, int32_t *size);


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

//! \brief Set the input 'value' with the contents of the input 'boolean'
//!
//! \param value Value object to store a value type.
//! \param boolean Input booleanvalue which shall be stored in the 'value' object.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if 'value' are NULL pointer
//!     or that the value type of 'value' is not DISIR_VALUE_TYPE_INTEGER.
//! \return DISIR_STATUS_OK if 'value' is successfully populated with the contents of 'boolean'
//!
enum disir_status
dx_value_set_boolean (struct disir_value *value, uint8_t boolean);

//! \brief Reterieve the boolean type stored in value
//!
//! \param[in] value Value object to retrieve the boolean value from.
//! \param[out] boolean Populated with the integer value of 'value' object.
//!
//! \return DISIR_STATUS_OK if the 'boolean' input pointer is populated with stored value.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input 'value' or 'boolean' are NULL pointers,
//!     or the value type of 'value' is not DISIR_VALUE_TYPE_INTEGER.
//!
enum disir_status
dx_value_get_boolean (struct disir_value *value, uint8_t *boolean);

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

