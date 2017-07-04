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

        ConfigReader (struct disir_instance *disir);

        virtual ~ConfigReader () {};

        //! \brief Read a disir_config from a istream
        enum disir_status
        unserialize (struct disir_config **config, std::istream& stream);


        //! \brief reads a disir_config from a json file
        enum disir_status
        unserialize (struct disir_config **config, const std::string Json);

        //! \brief Generates a disir_config object from json
        //!
        //! \param[in] config reference to the config object to be created
        //! \param[in] filepath path to the json config
        //!
        //! \return DPLUGIN_STATUS_OK on success
        //! \return DPLUGIN_IO_ERROR if filepath is invalid.
        //! \return DPLUGIN_PARSE_ERROR if json file has syntax errors.
        //!
        enum disir_status
        unserialize (struct disir_config **config, const char *filepath);

        //! \brief  Parses the config on path and returns its semantic version
        //!
        //! param[in] semver semantic version object populated
        //! param[in] path path to config file
        //! \return DPLUGIN_STATUS_OK on success
        //! \return DPLUGIN_IO_ERROR if filepath is invalid.
        //! \return DPLUGIN_PARSE_ERROR if json file has syntax errors.
        //!
        enum disir_status
        read_config_version (struct semantic_version *semver, const char *path);

    private:
        /* Methods */

        //! Function to read a jsonconfig from filepath
        virtual enum disir_status
            read_json_from_file (const char *filepath, Json::Value& root) {
                return JsonIO::read_json_from_file (filepath, root);
            }

        //! \brief Sets a version on the config
        //!
        //! If semantic version cannot be read, the version is set to 1.0.0
        //!
        //! \param[in] context_config the config to which the version is set.
        //! \param[in] ver json object containing the version string
        //!
        void set_config_version (struct disir_context *context_config, Json::Value& ver);

        //! \brief populates parent context with value type of keyval
        //!
        enum disir_status
        set_keyval (struct disir_context *parent_context,
                    std::string name,
                    Json::Value& keyval);

        enum disir_status
        unserialize_type (struct disir_context *context , Json::Value& value, std::string& name);

        enum disir_status
        unserialize_array (struct disir_context *parent, Json::Value& array,
                         std::string& name);

        //! \brief if duplicate keynames are postfixed, this function removes
        //! them before creating a disir_context object
        void remove_enumeration_postfix (std::string& name);

        //! \brief Constructs a disir_keyval object from a Json::Value object
        //! The parameter is of type iterator and not Value to provide the function with
        //! the keyname
        enum disir_status
        unserialize_keyval_entry (struct disir_context *parent_context,
                                Json::OrderedValueIterator& keyval_entry);

        //! \brief main function that handles the conversion from json to
        //! a disir_config object
        enum disir_status
        build_config_from_json (struct disir_context *context_config);

        //! \brief After unserialization into internal m_configRoot, this constructs the config.
        enum disir_status
        construct_config (struct disir_config **config);

        //! \brief recursively reads a json config starting from root and
        //! populated a disir_config accordingly
        //!
        //!
        enum disir_status
        _unserialize_node (struct disir_context *parent_context,
                         Json::Value& parent);

        //! \brief Resolves whether node is of type section
        bool value_is_section (Json::Value& node);

        //! \brief Resolves whether node is of type keyval
        bool value_is_keyval (Json::Value& node);

        /* Class members */

        //! holding the mold reference
        struct disir_mold *m_refMold;

        //! Variable to hold root json object
        Json::Value m_configRoot;
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
            //! - Errors are set on the disir instance
            //!
            //! \param[in] stream reference to the stream to read from
            //! \param[out] mold reference to where the constructed mold object is placed
            //!
            //! \return DISIR_STATUS_OK on success.
            //!
            enum disir_status
            unserialize (std::istream& stream, struct disir_mold **mold);

            enum disir_status
            unserialize (std::string mold_json, struct disir_mold **mold);

            //! \brief constructs a disir_mold object from a json file on filepath
            //!
            //! - Errors are set on the disir instance
            //!
            //! param[in] filepath path to where the json mold representation lies.
            //! param[in, out] mold reference to where the constructed mold object is placed
            //!
            //! \return DPLUGIN_STATUS_OK on success.
            //! \return DPLUGIN_FATAL_ERROR on unrecoverable errors.
            //! \return DPLUGIN_PARSE_ERROR if json object on filepath has syntax errors.
            //! \return DPLUGIN_IO_ERROR if file on filepath could not be read.
            //!
            enum disir_status
            unserialize (const char *filepath, struct disir_mold **mold);

            //! \brief set mold override
            enum disir_status
            set_mold_override (std::istream& stream);

            //! \brief check if a given json mold is a
            //! namespace override entry
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

            bool
            override_mold_is_set () { return override_mold_entries; }

            /* Methods */

            //! \brief construct the disir_mold based on already parsed m_moldRoot
            enum disir_status
            construct_mold (struct disir_mold **mold);

            //! \brief recursively extract jsonValue to create mold object.
            //!
            //!
            //!
            //! \return DPLUGIN_STATUS_OK on success.
            //! \return DPLUGIN_
            enum disir_status
            _unserialize_mold (struct disir_context *parent_context,
                                                 Json::Value& parent);

            //! \brief reads the json config from disk, located at filepath.
            enum disir_status
                read_json_from_file (const char *filepath, Json::Value& root) {
                    return JsonIO::read_json_from_file (filepath, root);
                }

            bool mold_has_documentation (Json::Value& mold_root);

            //! \brief Infers whether json value is a section
            //!
            //! \param[in] val json value that is checked.
            //!
            //! \return FALSE if val is not a section.
            //! \return TRUE if val is a section.
            //!
            bool value_is_section (Json::Value& val);

            //! \brief Infers whether json value is keyval
            //!
            //! \param[in] val json value that is checked.
            //!
            //! \return FALSE if val is not of type keyval.
            //! \return TRUE if val is of type keyval.
            //!
            bool value_is_keyval (Json::Value& val);

            //! \brief Extracts all default values and versions from the the json iterator object
            //!
            //! \param[in] child_context keyval context to contrain the defaults
            //! \param[in] it iterator object pointing to the json object mirroring the
            //! default keyval object.
            //!
            //! \return DPLUGIN_STATUS_OK on success.
            //! \return DPLUGIN_FATAL_ERROR if function needs to recover instantly
            //! from an error. Otherwise errors are added to disir instance.
            //!
            enum disir_status
            unserialize_defaults (struct disir_context *child_context,
                                Json::Value& current);

            //! \brief set introduced and documentation on Mold, keyval or section object.
            enum disir_status set_context_attributes (struct disir_context *context,
                                                      Json::OrderedValueIterator& json_context,
                                                      enum disir_context_type type);

            //! \brief get introduced version from json context object
            enum disir_status unserialize_introduced (struct disir_context *context,
                                                    Json::Value& current);

            enum disir_status unserialize_deprecated (struct disir_context *context,
                                                    Json::Value& current);

            enum disir_status set_restriction_value (struct disir_context *context,
                                                     Json::Value& current);

            //! \brief unserializes all restriction attached to a context
            enum disir_status unserialize_restrictions (struct disir_context *context,
                                                      Json::OrderedValueIterator& it);

            enum disir_status unserialize_restriction (struct disir_context *restriction,
                                                     Json::Value& current);

            enum disir_status unserialize_documentation (struct disir_context *context,
                                                       Json::Value& current);

            enum disir_status unserialize_context (struct disir_context *parent_context,
                                                  Json::OrderedValueIterator& current,
                                                  enum disir_context_type type);
    };

}

#endif

