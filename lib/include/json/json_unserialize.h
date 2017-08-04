#ifndef DIO_JSON_INPUT_H
#define DIO_JSON_INPUT_H

#include "dplugin_json.h"
#include <json/json.h>
#include "json/json_mold_override.h"

// cpp standard
#include <memory>

namespace dio
{
    //! Class that parses a disir config represented as json.
    class ConfigReader : public JsonIO
    {
    public:
        //! \brief config reader Constructor
        //!
        //! \param[in] disir the disir instance issuing the config input
        //! \param[in] mold The reference mold to the config requested
        //!
        ConfigReader (struct disir_instance *disir, struct disir_mold *mold);

        virtual ~ConfigReader () {};

        //! \brief Read a disir_config from a istream
        enum disir_status
        unserialize (struct disir_config **config, std::istream& stream);

        //! \brief reads a disir_config from a json file
        enum disir_status
        unserialize (struct disir_config **config, const std::string Json);

    private:

        //! \brief Sets a version on the config
        //!
        //! \param[in] context_config the config to which the version is set.
        //! \param[in] ver json object containing the version string
        //!
        enum disir_status
        set_config_version (struct disir_context *context_config, Json::Value& ver);

        //! \brief populates parent context with value type of keyval
        enum disir_status
        set_keyval (struct disir_context *parent_context, std::string name, Json::Value& keyval);

        //! \brief Unserializes primitives according to the json type
        enum disir_status
        unserialize_type (struct disir_context *context, Json::Value& value, std::string& name);

        //! \brief Unserialize multiple values of a keyval
        enum disir_status
        unserialize_array (struct disir_context *parent, Json::Value& array, std::string& name);

        //! \brief Constructs a disir_keyval object from a Json::Value object
        //! The parameter is of type iterator and not Value to provide the function with
        //! the keyname
        enum disir_status
        unserialize_keyval_entry (struct disir_context *parent_context,
                                  Json::OrderedValueIterator& keyval_entry);

        //! \brief After unserialization into internal m_configRoot, this constructs the config.
        enum disir_status
        construct_config (Json::Value& root, struct disir_config **config);

        //! \brief recursively reads a json config starting from root and
        //!     populated a disir_config accordingly
        enum disir_status
        _unserialize_node (struct disir_context *parent_context, Json::Value& parent);

    public:
        //! holding the mold reference
        struct disir_mold *m_mold;
    };

    //! Class that unmarshals a json mold representation into a mold object
    class MoldReader : public JsonIO
    {
        public:
            //! \brief Constructor
            MoldReader (struct disir_instance *disir);

            virtual ~MoldReader ();

            //! \brief construct a disir_mold from the JSON formatted data read from stream
            //!
            //! \param[in] stream reference to the stream to read from
            //! \param[out] mold reference to where the constructed mold object is placed
            //!
            //! \return DISIR_STATUS_OK on success.
            //! \return DISIR_STATUS_INVALID_CONTEXT if serialized mold
            //!     contains elements that are not according to spesification.
            //! \return DISIR_STATUS_FS_ERROR if json object is unparasable.
            //!
            enum disir_status
            unserialize (std::istream& stream, struct disir_mold **mold);

            //! \brief Construct a disir_mold from the JSON formatted data read from string
            //!
            //! \param[in] string string containing json mold
            //! \param[out] mold reference to where the constructed mold object is placed
            //!
            //! \return DISIR_STATUS_OK on success.
            //! \return DISIR_STATUS_INVALID_CONTEXT if serialized mold
            //!     contains elements that are not according to spesification.
            //! \return DISIR_STATUS_FS_ERROR if json object is unparasable.
            //!
            enum disir_status
            unserialize (std::string mold_json, struct disir_mold **mold);

            //! \brief Set mold override
            //!
            //! param[in] stream Mold override
            //!
            //! \return DISIR_STATUS_OK on success.
            //! \return DISIR_STATUS_NOT_EXIST if content of stream is not a mold override entry.
            //! \return DISIR_STATUS_FS_ERROR if mold override entry is invalid.
            //!
            enum disir_status
            set_mold_override (std::istream& stream);

            //! \brief Check if contents of entry is a mold override entry.
            //!
            //! param[in] entry Mold override.
            //!
            //! \return DISIR_STATUS_EXISTS on success.
            //! \return DISIR_STATUS_NOT_EXIST if content of stream is not a mold override entry.
            //! \return DISIR_STATUS_FS_ERROR if mold override entry is invalid.
            //!
            enum disir_status
            is_override_mold_entry (std::istream& entry);

        private:
            /* Members */
            struct disir_context *context_mold;
            Json::Value m_moldRoot;
            // True if override entry is provided
            bool override_mold_entries = false;
            // Instance of the override parser and applyer
            MoldOverride m_override_reader;

            //! \brief Returns whether override mold entry is set
            bool
            override_mold_is_set () { return override_mold_entries; }

            /* Methods */

            //! \brief construct the disir_mold based on already parsed m_moldRoot
            enum disir_status
            construct_mold (struct disir_mold **mold);

            //! \brief Recursively parse keyval/section elements of json mold
            enum disir_status
            _unserialize_mold (struct disir_context *parent_context, Json::Value& parent);

            //! \brief Check if mold_root contains documentation keyval.
            bool
            mold_has_documentation (Json::Value& mold_root);

            //! \brief Infers whether json value is a section.
            bool
            value_is_section (Json::Value& val);

            //! \brief Infers whether json value is keyval
            bool
            value_is_keyval (Json::Value& val);

            //! \brief Read all defaults from 'defaults' array
            enum disir_status
            unserialize_defaults (struct disir_context *child_context, Json::Value& current);

            //! \brief set introduced and documentation on Mold, keyval or section object.
            enum disir_status
            set_context_attributes (struct disir_context *context,
                                    Json::OrderedValueIterator& json_context,
                                    enum disir_context_type type);

            //! \brief Read introduced keyval from current object
            enum disir_status
            unserialize_introduced (struct disir_context *context, Json::Value& current);

            //! \brief Read depricated keyval from json object
            enum disir_status
            unserialize_deprecated (struct disir_context *context, Json::Value& current);

            //! \brief Set restriction value type
            enum disir_status
            set_restriction_value (struct disir_context *context, Json::Value& current);

            //! \brief unserializes all restriction attached to a context
            enum disir_status
            unserialize_restrictions (struct disir_context *context,
                                      Json::OrderedValueIterator& it);

            //! \brief Unserialize a single restriction
            enum disir_status
            unserialize_restriction (struct disir_context *restriction, Json::Value& current);

            //! \brief unserialize documentation keyval to context
            enum disir_status
            unserialize_documentation (struct disir_context *context, Json::Value& current);

            //! \brief Unserialize all attributes of either keyval or section.
            enum disir_status
            unserialize_context (struct disir_context *parent_context,
                                 Json::OrderedValueIterator& current,
                                 enum disir_context_type type);
    };

}

#endif

