// JSON private
#include "json/output.h"

// public
#include <disir/disir.h>

// standard
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>

using namespace dio;

ConfigWriter::ConfigWriter (struct disir_instance *disir)
    : JsonIO (disir)
{
    m_contextConfig = NULL;
}

ConfigWriter::~ConfigWriter ()
{
    if (m_contextConfig)
    {
        dc_putcontext (&m_contextConfig);
    }
}

enum dplugin_status
ConfigWriter::marshal (struct disir_config *config, std::ostream& stream)
{
    enum dplugin_status pstatus;
    std::string outputJson;
    Json::StyledWriter writer;

    pstatus = DPLUGIN_STATUS_OK;

    // Retrieving the config's context object
    m_contextConfig = dc_config_getcontext (config);
    if (m_contextConfig == NULL )
    {
        disir_error_set (m_disir, "could not retrieve cconfig context object");
        return DPLUGIN_FATAL_ERROR;
    }

    pstatus = set_config_version (m_contextConfig, m_configRoot);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto end;

    pstatus = _marshal_context (m_contextConfig, m_configRoot[CONFIG]);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto end;

    stream << writer.writeOrdered (m_configRoot);

end:
    dc_putcontext (&m_contextConfig);
    return pstatus;
}

enum dplugin_status
ConfigWriter::marshal (struct disir_config *config, std::string& output)
{
    enum dplugin_status pstatus;
    std::string outputJson;
    Json::StyledWriter writer;

    pstatus = DPLUGIN_STATUS_OK;

    // Retrieving the config's context object
    m_contextConfig = dc_config_getcontext (config);
    if (m_contextConfig == NULL )
    {
        disir_error_set (m_disir, "could not retrieve cconfig context object");
        return DPLUGIN_FATAL_ERROR;
    }

    pstatus = set_config_version (m_contextConfig, m_configRoot);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto end;

    pstatus = _marshal_context (m_contextConfig, m_configRoot[CONFIG]);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto end;

    output = writer.writeOrdered (m_configRoot);

end:
    dc_putcontext (&m_contextConfig);
    return pstatus;
}

enum dplugin_status
ConfigWriter::set_config_version (struct disir_context *context_config, Json::Value& root)
{
    struct semantic_version semver;
    char buf[500];
    char *temp;

    auto status = dc_get_version (context_config, &semver);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not read config version: (%s)",
                                   disir_status_string (status));
        return DPLUGIN_FATAL_ERROR;
    }

    temp = dc_semantic_version_string ((char *)buf, (int32_t)500, &semver);
    if (temp == NULL)
    {
        return DPLUGIN_FATAL_ERROR;
    }

    root[VERSION] = buf;

    return DPLUGIN_STATUS_OK;
}

// Mapping child node (section) to key (section name)
enum dplugin_status
ConfigWriter::set_section_keyname (struct disir_context *context, Json::Value& parent,
                                   Json::Value& sectionVal)
{
    std::string name = get_context_key (context);
    if (name.empty ())
    {
        // logged
        return DPLUGIN_FATAL_ERROR;
    }
    // If name already exists, enumerate it before
    // mapping section to parent
    if (parent[name].isNull () == false)
    {
        name = enumerate_keyname (parent, name);
    }

    // If section is empty
    // we just print an empty object
    if (sectionVal.isNull ())
    {
        sectionVal = Json::objectValue;
    }

    parent[name] = sectionVal;

    return DPLUGIN_STATUS_OK;
}

std::string
ConfigWriter::get_context_key (struct disir_context *context)
{
    const char *name;
    int32_t size;
    enum disir_status status;

    status = dc_get_name (context, &name, &size);
    if (status != DISIR_STATUS_OK) {
        // Should not happen
        disir_error_set (m_disir, "Disir returned an error from dc_get_name: %s",
                                   disir_status_string (status));
        return std::string ();
    }

    return std::string (name, size);
}

std::string
ConfigWriter::enumerate_keyname (Json::Value& node, const std::string name)
{
    std::string enumerated_name = name;
    std::string enumeration;
    int counter = 2;

    // Enumerate
    enumerated_name += "@";
    do {
        enumeration = enumerated_name;
        enumeration += std::to_string (counter);
        counter++;
    } while (node[enumeration].isNull () != true);

    return enumeration;
}

// Wraps libdisir dc_get_value to handle arbitrary value sizes
enum dplugin_status
ConfigWriter::set_keyval (struct disir_context *context, std::string name, Json::Value& node)
{
    enum disir_status status;
    enum disir_value_type type;
    int32_t size;
    double floatval;
    int64_t intval;
    const char *stringval;
    uint8_t boolval;
    std::string buf;
    Json::Value keyval;

    status = dc_get_value_type (context, &type);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not obtain context_value_type (%s)",
                                   disir_status_string (status));
        return DPLUGIN_FATAL_ERROR;
    }

    // The json rfc dictates that
    // no multiple entries shall exist.
    // Therefore, we store multiple keyvals
    // as a json array.
    if (node[name].isNull () == false)
    {
        name = enumerate_keyname (node, name);
    }

    switch (type) {
        case DISIR_VALUE_TYPE_STRING:
            status = dc_get_value_string (context, &stringval, &size);
            if (status != DISIR_STATUS_OK)
                goto error;

            keyval = stringval;
            break;
        case DISIR_VALUE_TYPE_INTEGER:
            status = dc_get_value_integer (context, &intval);
            if (status != DISIR_STATUS_OK)
                goto error;

            keyval = (Json::Int64)intval;
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            status = dc_get_value_float (context, &floatval);
            if (status != DISIR_STATUS_OK)
                goto error;

            keyval = floatval;
            break;
        case DISIR_VALUE_TYPE_BOOLEAN:
            status = dc_get_value_boolean (context, &boolval);
            if  (status != DISIR_STATUS_OK)
                goto error;

            keyval = !!boolval;
            break;
        case DISIR_VALUE_TYPE_ENUM:
            status = dc_get_value_enum (context, &stringval, NULL);
            if (status != DISIR_STATUS_OK)
                goto error;
            break;
        case DISIR_VALUE_TYPE_UNKNOWN:
            // If type is not know, we mark it
            // as unkwnown
            keyval = dc_value_type_string (context);
            break;
        default:
            // HUH? Type not supported?
            disir_error_set (m_disir, "Got an unsupported disir value type: %s",
                                       dc_value_type_string (context));
            keyval = dc_value_type_string (context);
            break;
    }

    node[name] = keyval;

    return DPLUGIN_STATUS_OK;
error:
    disir_error_set (m_disir, "Unable to fetch value from keyval with name: %s and type %s",
                               name.c_str(), dc_value_type_string (context));
    return DPLUGIN_FATAL_ERROR;
}

enum dplugin_status
ConfigWriter::marshal_keyval (struct disir_context *context, Json::Value& node)
{
   std::string name = get_context_key (context);
   if (name.empty ())
   {
       return DPLUGIN_FATAL_ERROR;
   }

   return set_keyval (context, name, node);
}

enum dplugin_status
ConfigWriter::_marshal_context (struct disir_context *parent_context, Json::Value& parent)
{
    struct disir_collection *collection;
    struct disir_context *child_context;
    enum disir_status status;
    enum dplugin_status pstatus;
    Json::Value child;

    status = dc_get_elements (parent_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    pstatus = DPLUGIN_STATUS_OK;

    while (dc_collection_next (collection, &child_context)
           != DISIR_STATUS_EXHAUSTED)
    {
        switch (dc_context_type (child_context)) {
            case DISIR_CONTEXT_SECTION:
                pstatus = _marshal_context (child_context, child);
                if (pstatus != DPLUGIN_STATUS_OK)
                {
                    goto end;
                }
                pstatus = set_section_keyname (child_context, parent, child);
                if (pstatus != DPLUGIN_STATUS_OK)
                {
                    // logged
                    return pstatus;
                }
                break;
            case DISIR_CONTEXT_KEYVAL:
                pstatus = marshal_keyval (child_context, parent);
                if (pstatus != DPLUGIN_STATUS_OK)
                    goto end;

                break;
            default:
                // should not happen
                // Informing disir about the error
                append_disir_error ("Config contained unsupported context (%s)",
                                     dc_context_type_string (child_context));
        }

        dc_putcontext (&child_context);
    }

end:
    if (child_context)
    {
        dc_putcontext (&child_context);
    }

    dc_collection_finished (&collection);

    return pstatus;
}

