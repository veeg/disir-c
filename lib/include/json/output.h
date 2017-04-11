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

       //! \brief Marhalls a disir_config object to json
       //!
       //! \param[config] config config object that is marhsalled to json
       //! \param[output] output string object to contain the config represented as json
       //!
       //! Certain errors are set on the disir instance
       //!
       //! \return DPLUGIN_STATUS_OK on success.
       //! \return DPLUGIN_FATAL_ERROR on unrecoverable errors.
       //!
       enum disir_status
       marshal (struct disir_config *config, std::string& output);

       //! \brief Marshal a disir_config object to JSON, writing it to the ostream
       enum disir_status
       marshal (struct disir_config *config, std::ostream& stream);


    private:
       // Variables
       Json::Value m_configRoot;
       struct disir_context *m_contextConfig;

       //! \brief Retrieves the name of a given context
       //!
       //! \param[in] context
       //!
       //! \return name of context on success.
       //! \return empty string on failure (Logged to disir).
       //!
       enum disir_status
       get_context_key (struct disir_context *context, std::string& key);

       //! \brief Insering a section onto its respective parent
       //!
       //! On multiple keynames, the section name is enumerated with the
       //! postfix @ followed by the number.
       //!
       //! \param[in] context
       //! \param[in] parent
       //! \param[in] sectionVal
       //!
       //! \return DPLUGIN_STATUS_OK on success.
       //! \return DPLUGIN_FATAL_ERROR if we cannot retrieve the value store in context
       //!
       enum disir_status
       set_section_keyname (struct disir_context *context,
                            Json::Value& parent,
                            Json::Value& sectionVal);

        //! \brief Insering a keyval onto its respective parent
       //!
       //! On multiple keynames, the keyval name is enumerated with the
       //! postfix @ followed by the number.
       //!
       //! \param[in] context
       //! \param[in] parent
       //! \param[in] sectionVal
       //!
       //! \return DISIR_STATUS_OK on success.
       //! \return DPLUGIN_FATAL_ERROR if we cannot retrieve the value store in context
       //!
       enum disir_status
       set_keyval (struct disir_context *context, std::string name, Json::Value& node);

       //! \brief parses a json object mirroring a disir_keyval and
       //!  generates a new config_keyval object.
       //!
       enum disir_status
       marshal_keyval (struct disir_context *context, Json::Value& parent);

       //! \brief sets the disir_config version
       enum disir_status
       set_config_version (struct disir_context *context_config,
                                                Json::Value& root);

       //! \brief recursive function that extracts an entire config starting from root
       enum disir_status
       _marshal_context (struct disir_context *parent_context,
                                              Json::Value& parent);

       //! \brief if a config has keyvals with identical names
       //! under the same root, we enumerate their names.
       std::string enumerate_keyname (Json::Value& node,
                                      const std::string name);
    };

    // Class implementing outputting a
    // disir_mold object in json
    class MoldWriter : public JsonIO
    {
    public:
       //! \brief
       enum dplugin_status marshal (struct disir_mold *mold, std::string& mold_json);

       //! \brief
       enum dplugin_status marshal (struct disir_mold *mold, std::ostream& stream);

       //! \brief Constructor
       MoldWriter (struct disir_instance *disir);

       virtual ~MoldWriter () {};
    private:

     //! \brief Fetches introduced type and documentation from context
     //!
     //! - Type is only set on keyval
     //!
     //! \param[in] context can be section keyval or mold.
     //! \param[in] element jsov value where data is inserted.
     //!
     void extract_context_metadata (struct disir_context *context, Json::Value& element);

     //! \brief constructs a json representation of a disir_mold keyval object
     //!
     //! \param[in] context_keyval keyval context object marshalled
     //! \param[in] keyval json value where keyval data is inserted.
     //!
     //! \return DISIR_STATUS_OK on success.
     //! \return DISIR_FATAL_ERROR on unrecoverable errors.
     //!
     enum dplugin_status marshal_mold_keyval (struct disir_context *context_keyval,
                                             Json::Value& keyval);

    //! \brief recursively marshals all mold child context
    //!
    //! \param[in] parent_context current context node from which all childs are marshaled.
    //! \param[in] parent json object to which parent_contexts childs are inserted.
    //!
    //! \return DPLUGIN_STATUS_OK on success.
    //! \return DPLUGIN_FATAL_ERROR if unrecoverable errors occur.
    //!
    enum dplugin_status _marshal_mold_contexts (struct disir_context *parent_context,
                                                Json::Value& parent);

    //! \brief creates a json object based on one default context
    //!
    //! param[in] context The default context object.
    //! param[in] defaults Json array where context object is appended
    //!
    //! \return DPLUGIN_STATUS_OK on success.
    //! \return DPLUGIN_FATAL_ERROR if name or value could not be retrieved from context.
    //!
    enum dplugin_status get_default (struct disir_context *context, Json::Value& defaults);


       /* MEMBERS */

       struct disir_mold *m_mold;
    };
}

#endif

