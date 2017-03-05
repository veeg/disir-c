#ifndef DIO_JSON_H
#define DIO_JSON_H
#include <iostream>
#include "status_codes.h"
#include <json/json.h>
#include <disir/disir.h>

//! Json keynames
const char DOCUMENTATION[] = "doc";
const char DEFAULTS[] = "defaults";
const char INTRODUCED[] = "introduced";
const char VERSION[] = "version";
const char ELEMENTS[] = "elements";
const char MOLD[] = "mold";
const char VALUE[] = "value";
const char TYPE[] = "type";
const char CONFIG[] = "config";

enum dplugin_status
object_members_check (Json::Value& object, ...);

#define MEMBERS_CHECK(object, ...) \
        object_members_check (object, __VA_ARGS__, 0)

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
        virtual enum dplugin_status read_config (const char *filepath, Json::Value& root);

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
