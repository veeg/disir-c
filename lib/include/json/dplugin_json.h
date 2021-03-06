#ifndef DIO_JSON_H
#define DIO_JSON_H
#include <iostream>
#include <json/json.h>
#include <disir/disir.h>
#include <map>

//! Attribute keys for json mold and config
#define ATTRIBUTE_KEY_DOCUMENTATION "documentation"
#define ATTRIBUTE_KEY_INTRODUCED "introduced"
#define ATTRIBUTE_KEY_DEPRECATED "deprecated"
#define ATTRIBUTE_KEY_VERSION "version"
#define ATTRIBUTE_KEY_ELEMENTS "elements"
#define ATTRIBUTE_KEY_DEFAULTS "defaults"
#define ATTRIBUTE_KEY_MOLD "mold"
#define ATTRIBUTE_KEY_VALUE "value"
#define ATTRIBUTE_KEY_TYPE "type"
#define ATTRIBUTE_KEY_CONFIG "config"
#define ATTRIBUTE_KEY_RESTRICTIONS "restrictions"

//! Attribute keys for mold override
#define ATTRIBUTE_KEY_SYNC "sync"
#define ATTRIBUTE_KEY_OVERRIDE "override"
#define ATTRIBUTE_KEY_NAMESPACE "namespace"

//! set value on context according to typeof val
enum disir_status
set_value (struct disir_context *context, Json::Value& val);

//! Overwrite default value on version
enum disir_status
add_value_default (struct disir_context *contexts, Json::Value& value,
                   struct disir_version *version);

//! \brief resolve disir_value_type from its string representation
enum disir_value_type
attribute_key_to_disir_value (const char *type);

//! \brief resolve disir_restriction_type from its string representation
enum disir_restriction_type
attribute_key_to_disir_restriction (const char *type);

//! \brief determine if value is of a certain jsonValueType
enum disir_status
assert_json_value_type (Json::Value& value, Json::ValueType type);

//! Get the string representation of a json ValueType
//! return "unknown" if called with invalid type
const char *
json_valuetype_stringify (Json::ValueType type);


namespace dio
{
    //! \brief Parent class to all readers and writers
    //! in this IO plugin
    class JsonIO
    {
    public:
        //!  \brief constructor for mold input
        //!
        //! \param[in] disir the disir intance issuing an IO request
        //!
        JsonIO (struct disir_instance *disir);

        virtual ~JsonIO () = default;
   protected:
        // An instance of the libdisir instance
        // that invoked library call
        struct disir_instance *m_disir;
    };
}

#endif
