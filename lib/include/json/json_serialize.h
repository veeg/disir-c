#ifndef DIO_JSON_OUTPUT_H
#define DIO_JSON_OUTPUT_H

#include "dplugin_json.h"
#include <json/json.h>

namespace dio
{
    // A Class implementing o a
    // disir_config object in json
    class ConfigWriter : public JsonIO
    {
    public:

        ConfigWriter (struct disir_instance *disir);
        virtual ~ConfigWriter ();

        //! \brief Serialize config to json
        //!
        //! \param[in] config Config object to be serialize.
        //! \param[out] output String that will be conatined the serialized config.
        //!
        //! \return DISIR_STATUS_OK on success.
        //!
        enum disir_status
        serialize (struct disir_config *config, std::string& output);

        //! \brief Serialize config to json
        //!
        //! \param[in] config Config object to be serialize.
        //! \param[out] stream Stream that will be conatined the serialized config.
        //!
        //! \return DISIR_STATUS_OK on success.
        //!
        enum disir_status
        serialize (struct disir_config *config, std::ostream& stream);

     private:
        // Variables
        Json::Value m_configRoot;
        struct disir_context *m_contextConfig;

        //! \brief Retrive context name
        enum disir_status
        get_context_key (struct disir_context *context, std::string& key);

        //! \brief Serialize section name
        enum disir_status
        set_section_keyname (struct disir_context *context, Json::Value& parent,
                             Json::Value& sectionVal);

        // \brief Serialize keyval value
        enum disir_status
        set_keyval (struct disir_context *context, std::string name, Json::Value& node);

        // \brief Serialize keyval name and value
        enum disir_status
        serialize_keyval (struct disir_context *context, Json::Value& parent);

        //! \brief Serialize config version
        enum disir_status
        set_config_version (struct disir_context *context_config, Json::Value& root);

        //! \brief recursive function that extracts an entire config starting from root
        enum disir_status
        _serialize_context (struct disir_context *parent_context, Json::Value& parent);

        //! \brief if a config has keyvals or sections with identical names
        //! under the same root, we merge them into an array.
        void
        serialize_duplicate_entries (Json::Value& parent, Json::Value& child,
                                     const std::string name);
    };

    // Class implementing outputting a
    // disir_mold object in json
    class MoldWriter : public JsonIO
    {
    public:
        //! \brief Serialize disir_mold
        //!
        //! \param[in] mold The disir mold.
        //! \param[out] out String that will contain serialized mold.
        //!
        //! \return DISIR_STATUS_OK on success.
        //!
        enum disir_status
        serialize (struct disir_mold *mold, std::string& out);

        //! \brief Serialize disir_mold
        //!
        //! \param[in] mold The disir mold.
        //! \param[out] stream Stream that will contain serialized mold.
        //!
        //! \return DISIR_STATUS_OK on success.
        //!
        enum disir_status
        serialize (struct disir_mold *mold, std::ostream& stream);

        //! \brief Constructor
        MoldWriter (struct disir_instance *disir);

        virtual ~MoldWriter () {};
    private:
        //! \brief Serialize introduced version.
        enum disir_status
        serialize_introduced (struct disir_context *context, Json::Value& element);

        //! \brief Serialize depricated version.
        enum disir_status
        serialize_deprecated (struct disir_context *context, Json::Value& element);

        //! \brief Serialize mold keyval
        enum disir_status
        serialize_mold_keyval (struct disir_context *context_keyval, Json::Value& keyval);

        //! \brief Serialize mold restriction type
        enum disir_status
        serialize_restrictions (struct disir_context *context, Json::Value& restriction);

        //! \brief Serialize context attributes, depending on its spesification
        enum disir_status
        serialize_attributes (struct disir_context *context, Json::Value& current,
                              enum disir_context_type type);

        //! \brief Recursively serialize context based on type
        enum disir_status
        _serialize_mold_contexts (struct disir_context *parent_context, Json::Value& parent);

        //! \brief fetch all default contexts from keyval and serialize them into an array
        enum disir_status
        serialize_default (struct disir_context *context, Json::Value& defaults);

        /* MEMBERS */
        struct disir_mold *m_mold = nullptr;
    };
}

#endif // DIO_JSON_OUTPUT

