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

enum disir_status
ConfigWriter::marshal (struct disir_config *config, std::ostream& stream)
{
    enum disir_status status;
    Json::StyledWriter writer;
    std::string outputJson;

    status = DISIR_STATUS_OK;

    // Retrieving the config's context object
    m_contextConfig = dc_config_getcontext (config);
    if (m_contextConfig == NULL )
    {
        disir_error_set (m_disir, "could not retrieve cconfig context object");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = set_config_version (m_contextConfig, m_configRoot);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    status = _marshal_context (m_contextConfig, m_configRoot[ATTRIBUTE_KEY_CONFIG]);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    stream << writer.writeOrdered (m_configRoot);

end:
    dc_putcontext (&m_contextConfig);
    return status;
}

enum disir_status
ConfigWriter::marshal (struct disir_config *config, std::string& output)
{
    enum disir_status status;
    Json::StyledWriter writer;
    std::string outputJson;

    // Retrieving the config's context object
    m_contextConfig = dc_config_getcontext (config);
    if (m_contextConfig == NULL )
    {
        disir_error_set (m_disir, "could not retrieve cconfig context object");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = set_config_version (m_contextConfig, m_configRoot);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    status = _marshal_context (m_contextConfig, m_configRoot[ATTRIBUTE_KEY_CONFIG]);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    output = writer.writeOrdered (m_configRoot);


end:
    dc_putcontext (&m_contextConfig);
    return status;
}

enum disir_status
ConfigWriter::set_config_version (struct disir_context *context_config, Json::Value& root)
{
    struct semantic_version semver;
    enum disir_status status;
    char buf[500];
    char *temp;

    status = dc_get_version (context_config, &semver);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not read config version: (%s)",
                                   disir_status_string (status));
        return status;
    }

    temp = dc_semantic_version_string ((char *)buf, (int32_t)500, &semver);
    if (temp == NULL)
    {
        disir_error_set (m_disir, "Error retrieving semantic version string");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    root[ATTRIBUTE_KEY_VERSION] = buf;

    return status;
}

// Mapping child node (section) to key (section name)
enum disir_status
ConfigWriter::set_section_keyname (struct disir_context *context, Json::Value& parent,
                                   Json::Value& sectionVal)
{
    std::string name;
    enum disir_status status;

    status = get_context_key (context, name);
    if (status != DISIR_STATUS_OK)
    {
        // logged
        return status;
    }

    serialize_duplicate_entries (parent, sectionVal, name);

    return status;
}

enum disir_status
ConfigWriter::get_context_key (struct disir_context *context, std::string& key)
{
    const char *name;
    int32_t size;
    enum disir_status status;

    status = dc_get_name (context, &name, &size);
    if (status != DISIR_STATUS_OK) {
        // Should not happen
        disir_error_set (m_disir, "Disir returned an error from dc_get_name: %s",
                                   disir_status_string (status));
        return status;
    }

    key = std::string (name, size);

    return status;
}

void
ConfigWriter::serialize_duplicate_entries (Json::Value& parent,
                                           Json::Value& child, const std::string name)
{
    Json::Value entries;

    if (parent[name].isArray())
    {
        parent[name].append (child);
    }
    else if (parent[name].isNull() == false)
    {
        entries = Json::arrayValue;
        entries.append (parent[name]);
        entries.append (child);
        child = entries;
        parent[name] = child;
    }
    else
    {
        parent[name] = child;
    }
}

// Wraps libdisir dc_get_value to handle arbitrary value sizes
enum disir_status
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
        return status;
    }

    switch (type) {
        case DISIR_VALUE_TYPE_STRING:
            status = dc_get_value_string (context, &stringval, &size);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }

            keyval = stringval;
            break;
        case DISIR_VALUE_TYPE_INTEGER:
            status = dc_get_value_integer (context, &intval);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }

            keyval = (Json::Int64)intval;
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            status = dc_get_value_float (context, &floatval);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }

            keyval = floatval;
            break;
        case DISIR_VALUE_TYPE_BOOLEAN:
            status = dc_get_value_boolean (context, &boolval);
            if  (status != DISIR_STATUS_OK)
            {
                goto error;
            }

            keyval = !!boolval;
            break;
        case DISIR_VALUE_TYPE_ENUM:
            status = dc_get_value_enum (context, &stringval, NULL);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }
            keyval = stringval;
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
            break;
    }

    serialize_duplicate_entries (node, keyval, name);

    return status;
error:
    disir_error_set (m_disir, "Unable to fetch value from keyval with name: %s and type %s",
                               name.c_str(), dc_value_type_string (context));
    return status;
}

enum disir_status
ConfigWriter::marshal_keyval (struct disir_context *context, Json::Value& node)
{
    enum disir_status status;
    std::string name;
    status = get_context_key (context, name);
    if (status != DISIR_STATUS_OK)
    {
       return status;
    }

    return set_keyval (context, name, node);
}

enum disir_status
ConfigWriter::_marshal_context (struct disir_context *parent_context, Json::Value& parent)
{
    struct disir_collection *collection;
    struct disir_context *child_context;
    enum disir_status status;

    status = dc_get_elements (parent_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto end;
    }

    status = DISIR_STATUS_OK;

    while (dc_collection_next (collection, &child_context)
           != DISIR_STATUS_EXHAUSTED)
    {
        // per-iteration such that child values
        // never have old references to pre-marshaled
        // context objects
        Json::Value child;

        switch (dc_context_type (child_context))
        {
            case DISIR_CONTEXT_SECTION:

                status = _marshal_context (child_context, child);
                if (status != DISIR_STATUS_OK)
                {
                    goto end;
                }

                status = set_section_keyname (child_context, parent, child);
                if (status != DISIR_STATUS_OK)
                {
                    // logged
                    return status;
                }
                break;
            case DISIR_CONTEXT_KEYVAL:
                status = marshal_keyval (child_context, parent);
                if (status != DISIR_STATUS_OK)
                {
                    goto end;
                }
                break;
            default:
                // should not happen
                // Informing disir about the error
                append_disir_error ("Config contained unsupported context (%s)",
                                     dc_context_type_string (child_context));
                break;
        }
        dc_putcontext (&child_context);
    }

end:
    if (child_context)
    {
        dc_putcontext (&child_context);
    }

    dc_collection_finished (&collection);

    return status;
}

