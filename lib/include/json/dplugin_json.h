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
#define ATTRIBUTE_KEY_VALUE_MIN "value_min"
#define ATTRIBUTE_KEY_VALUE_MAX "value_max"
#define ATTRIBUTE_KEY_TYPE "type"
#define ATTRIBUTE_KEY_CONFIG "config"
#define ATTRIBUTE_KEY_RESTRICTIONS "restrictions"

//! \brief helper function that resolves typeof val and sets context's value accordingly
enum disir_status set_value (Json::Value& val, struct disir_context *context);

//! \brief resolve disir_value_type from its string representation
enum disir_value_type attribute_key_to_disir_value (const char *type);

//! \brief resolve disir_restriction_type from its string representation
enum disir_restriction_type attribute_key_to_disir_restriction (const char *type);

enum disir_status assert_json_value_type (Json::Value& value, Json::ValueType type);


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

        //! \brief reads a file from filepath and parses it into the json value object
        //!
        //! \param[in] filepath path to where the file resides
        //! \param[in] root object to which filecontent is parsed
        //!
        //! \return DPLUGIN_STATUS_OK on success
        //! \return DPLUGIN_PARSE_ERROR if the json file contains errors. The disir insance
        //! will contain an explenation of any errors.
        //! \return DPLUGIN_IO_ERROR if filepath is invalid.
        //!
        virtual enum disir_status read_json_from_file (const char *filepath, Json::Value& root);

   protected:
        // An instance of the libdisir instance
        // that invoked library call
        struct disir_instance *m_disir;
    };
}

#endif
