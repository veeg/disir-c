#ifndef DIO_UTIL_H
#define DIO_UTIL_H


//! \brief helper function that resolves typeof val and sets context's value accordingly
enum disir_status set_value (Json::Value& val, struct disir_context *context);

//! \brief resolve disir_value_type from its string representation
enum disir_value_type string_to_type (std::string type);


#endif // DIO_UTIL_H
