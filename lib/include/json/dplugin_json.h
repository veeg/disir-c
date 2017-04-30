#ifndef DIO_JSON_H
#define DIO_JSON_H
#include <iostream>
#include <json/json.h>
#include <disir/disir.h>

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

enum disir_status
object_members_check (struct disir_instance *intance, Json::Value& object, ...);

#define ASSERT_MEMBERS_PRESENT(instance, object, ...) \
        object_members_check (instance, object, __VA_ARGS__, 0)


//! \brief helper function that resolves typeof val and sets context's value accordingly
enum disir_status set_value (Json::Value& val, struct disir_context *context);

//! \brief resolve disir_value_type from its string representation
enum disir_value_type string_to_type (std::string type);


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
        virtual enum disir_status read_config (const char *filepath, Json::Value& root);

   protected:
        //! \brief populates an internal errorlog
        void add_error (const char *prefix, const char *message, va_list args);

        //! Logging an error on an object where the keyname is needed, hence ValueIterator
        //! and not just Value
        void append_disir_error (Json::ValueIterator& object, const char *message, ...);

        //! \brief Logging an errormessage to a list, later to be appended to the underlying disir
        //! instance
        //!
        //! \param[in] Object The iterator object, used to log a section or keyval name
        //!
        void append_disir_error (Json::OrderedValueIterator& object, const char *message, ...);

        //! \brief appends an error message to an error list later to be set on the underlying
        //! disir instance
        //!
        //! \param[in] message Error message
        //!
        void append_disir_error (const char *message, ...);

        //! \brief concatenates all errormessages and populates
        //! the current disir instance with it
        void populate_disir_with_errors ();

        // An instance of the libdisir instance
        // that invoked library call
        struct disir_instance *m_disir;

        //! \brief errors
        std::vector<std::string> m_errors;

    };
}

#endif
