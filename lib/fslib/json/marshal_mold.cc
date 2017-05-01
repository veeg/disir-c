// JSON private
#include "json/dplugin_json.h"
#include "json/output.h"

// public
#include <disir/disir.h>

// standard
#include <iostream>

using namespace dio;

MoldWriter::MoldWriter (struct disir_instance *disir) : JsonIO (disir)
{
}


enum disir_status
MoldWriter::serialize_deprecated (struct disir_context *context, Json::Value& current)
{
    char buf[500];
    enum disir_status status;
    struct semantic_version semver;

    status = dc_get_deprecated (context, &semver);
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_WRONG_CONTEXT)
    {
        return status;
    }
    if (status == DISIR_STATUS_WRONG_CONTEXT)
    {
        return DISIR_STATUS_OK;
    }

    auto semver_string = dc_semantic_version_string (buf, 500, &semver);

    current[ATTRIBUTE_KEY_DEPRECATED] = semver_string;

    return status;
}

enum disir_status
MoldWriter::serialize_introduced (struct disir_context *context, Json::Value& current)
{
    char buf[500];
    enum disir_status status;
    struct semantic_version semver;

    status = dc_get_introduced (context, &semver);
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_WRONG_CONTEXT)
    {
        return status;
    }
    if (status == DISIR_STATUS_WRONG_CONTEXT)
    {
        return DISIR_STATUS_OK;
    }

    auto semver_string = dc_semantic_version_string (buf, 500, &semver);

    current[ATTRIBUTE_KEY_INTRODUCED] = semver_string;

    return status;
}

enum disir_status
MoldWriter::marshal (struct disir_mold *mold, std::string& mold_json)
{
    struct disir_context *context_mold;
    enum disir_status status;
    Json::StyledWriter writer;
    Json::Value root;

    context_mold = dc_mold_getcontext (mold);
    if (context_mold == NULL)
    {
        disir_error_set (m_disir, "Could not retrieve context mold from mold");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = serialize_attributes (context_mold, root, DISIR_CONTEXT_MOLD);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = _serialize_mold_contexts (context_mold, root[ATTRIBUTE_KEY_MOLD]);
    if (status != DISIR_STATUS_OK)
        goto end;

    mold_json = writer.writeOrdered (root);

end:
    dc_putcontext (&context_mold);
    return status;
}

enum disir_status
MoldWriter::marshal (struct disir_mold *mold, std::ostream& stream)
{
    struct disir_context *context_mold;
    enum disir_status status;
    Json::StyledWriter writer;
    Json::Value root;

    context_mold = dc_mold_getcontext (mold);
    if (context_mold == NULL)
    {
        disir_error_set (m_disir, "Could not retrieve context mold from mold");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    status = serialize_attributes (context_mold, root, DISIR_CONTEXT_MOLD);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = _serialize_mold_contexts (context_mold, root[ATTRIBUTE_KEY_MOLD]);
    if (status != DISIR_STATUS_OK)
        goto end;

    stream << writer.writeOrdered (root);

end:
    dc_putcontext (&context_mold);
    return status;
}

enum disir_status
MoldWriter::serialize_attributes (struct disir_context *context, Json::Value& current,
                                  enum disir_context_type type)
{
    enum disir_status status;
    const char *doc;

    switch (type)
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        status = serialize_deprecated (context, current);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        current[ATTRIBUTE_KEY_TYPE] = dc_value_type_string (context);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_RESTRICTION:
    {
        status = serialize_deprecated (context, current);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        status = serialize_introduced (context, current);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        break;
    }
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_FREE_TEXT:
    case DISIR_CONTEXT_UNKNOWN:
    case DISIR_CONTEXT_DOCUMENTATION:
        break;
    }

    status = dc_get_documentation (context, NULL, &doc, NULL);
    if (status == DISIR_STATUS_OK)
    {
        //! Doc is set only when present in context
        current[ATTRIBUTE_KEY_DOCUMENTATION] = doc;
    }
    return DISIR_STATUS_OK;
}

enum disir_status
MoldWriter::serialize_mold_keyval (struct disir_context *context_keyval, Json::Value& keyval)
{
    enum disir_status status;
    struct disir_collection *coll;
    struct disir_context *context;
    Json::Value defaults;

    status = dc_get_default_contexts (context_keyval, &coll);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not acquire defualt context on keyval. Error (%s)",
                                   disir_status_string (status));
        goto out;
    }

    status = serialize_attributes (context_keyval, keyval, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    while (dc_collection_next (coll, &context)
            != DISIR_STATUS_EXHAUSTED)
    {
        status = serialize_default (context, defaults);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        dc_putcontext (&context);
    }

    status = serialize_restrictions (context_keyval, keyval);
    if (status != DISIR_STATUS_OK)
    {
        goto out;
    }

    keyval[ATTRIBUTE_KEY_DEFAULTS] = defaults;
out:
     dc_collection_finished (&coll);

     return status;
}

enum disir_status
MoldWriter::serialize_restrictions (struct disir_context *context, Json::Value& current)
{
    enum disir_status status;
    enum disir_restriction_type rtype;
    struct disir_collection *collection;
    struct disir_context *restriction;
    enum disir_value_type value_type;
    Json::Value restrictions = Json::arrayValue;

    status = dc_restriction_collection (context, &collection);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
    {
        disir_error_set (m_disir , "Error querying context for restrictions: %s",
                                   disir_status_string (status));
        return status;
    }
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        return DISIR_STATUS_OK;
    }

    while (dc_collection_next (collection, &restriction) != DISIR_STATUS_EXHAUSTED)
    {
        Json::Value current_restriction;

        status = serialize_attributes (restriction, current_restriction,
                                       DISIR_CONTEXT_RESTRICTION);
        if (status != DISIR_STATUS_OK)
        {
            goto out;
        }

        status = dc_get_restriction_type (restriction, &rtype);
        if (status !=  DISIR_STATUS_OK)
        {

            goto out;
        }

        auto restriction_enum_string = dc_restriction_enum_string (rtype);

        current_restriction[ATTRIBUTE_KEY_TYPE] = restriction_enum_string;

        dc_get_value_type (context, &value_type);

        switch (rtype)
        {
        case DISIR_RESTRICTION_INC_ENTRY_MIN:
        case DISIR_RESTRICTION_INC_ENTRY_MAX:
            value_type = DISIR_VALUE_TYPE_INTEGER;
        case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
        {
            double value;

            status = dc_restriction_get_numeric (restriction, &value);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            if (value_type == DISIR_VALUE_TYPE_FLOAT)
            {
                current_restriction[ATTRIBUTE_KEY_VALUE] = value;
            }
            else
            {
                current_restriction[ATTRIBUTE_KEY_VALUE] = (Json::Int64)value;
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_RANGE:
        {
            double min, max;

            status = dc_restriction_get_range (restriction, &min, &max);
            if (status != DISIR_STATUS_OK)
            {
                goto out;
            }

            if (value_type == DISIR_VALUE_TYPE_FLOAT)
            {
                current_restriction[ATTRIBUTE_KEY_VALUE_MIN] = min;
                current_restriction[ATTRIBUTE_KEY_VALUE_MAX] = max;
            }
            else
            {
                current_restriction[ATTRIBUTE_KEY_VALUE_MIN] = (Json::Int64)min;
                current_restriction[ATTRIBUTE_KEY_VALUE_MAX] = (Json::Int64)max;
            }

            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_ENUM:
        {
            const char *enum_value;

            status = dc_restriction_get_string (restriction, &enum_value);
            if (status != DISIR_STATUS_OK)
            {

                goto out;
            }

            current_restriction[ATTRIBUTE_KEY_VALUE] = enum_value;

            break;
        }
        case DISIR_RESTRICTION_UNKNOWN:
            disir_error_set (m_disir, "Got unknown restriction type %s\n",
                                       restriction_enum_string);

            status = DISIR_STATUS_INVALID_CONTEXT;
            break;
        default:
            break;
        }

        restrictions.append (current_restriction);

        dc_putcontext(&restriction);
    }

    current[ATTRIBUTE_KEY_RESTRICTIONS] = restrictions;
out:
    dc_collection_finished (&collection);
    return status;
}

enum disir_status
MoldWriter::_serialize_mold_contexts (struct disir_context *parent_context, Json::Value& parent)
{
    struct disir_collection *coll;
    struct disir_context *context;
    enum disir_status status;
    Json::Value child;
    const char *name;
    int32_t size;

    status = DISIR_STATUS_OK;

    status = dc_get_elements (parent_context, &coll);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    while (dc_collection_next (coll, &context)
            != DISIR_STATUS_EXHAUSTED)
    {
        switch (dc_context_type (context))
        {
            case DISIR_CONTEXT_KEYVAL:
                status = serialize_mold_keyval (context, child);
                if (status != DISIR_STATUS_OK)
                    goto end;

                break;
            case DISIR_CONTEXT_SECTION:
                status = serialize_attributes (context, child, DISIR_CONTEXT_SECTION);
                if (status != DISIR_STATUS_OK)
                {
                    return status;
                }

                status = serialize_restrictions (context, child);
                if (status != DISIR_STATUS_OK)
                {
                    return status;
                }

                status = _serialize_mold_contexts (context, child[ATTRIBUTE_KEY_ELEMENTS]);
                if (status != DISIR_STATUS_OK)
                    goto end;

                break;
            default:
                disir_error_set (m_disir, "Got unrecognizable context object: %s",
                                           dc_value_type_string (context));
                goto end;
        }

        status = dc_get_name (context, &name, &size);
        if (status != DISIR_STATUS_OK)
            goto end;

        parent[name] = child;
        child = Json::nullValue;

        dc_putcontext (&context);
    }
end:
    if (context)
    {
        dc_putcontext (&context);
    }

    dc_collection_finished (&coll);

    return status;
}

// Wraps libdisir dc_get_value to handle arbitrary value sizes
enum disir_status
MoldWriter::serialize_default (struct disir_context *context, Json::Value& defaults)
{
    enum disir_status status;
    enum disir_value_type type;
    int32_t size;
    double floatval;
    int64_t intval;
    const char *stringval;
    uint8_t boolval;
    std::string buf;
    Json::Value current;

    status = dc_get_value_type (context, &type);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not obtain context_value_type (%s)",
                                   disir_status_string (status));
        goto error;
    }

    switch (type) {
        case DISIR_VALUE_TYPE_STRING:
            status = dc_get_value_string (context, &stringval, &size);
            if (status != DISIR_STATUS_OK)
                goto error;

            current[ATTRIBUTE_KEY_VALUE] = stringval;
            break;
        case DISIR_VALUE_TYPE_INTEGER:
            status = dc_get_value_integer (context, &intval);
            if (status != DISIR_STATUS_OK)
                goto error;

            current[ATTRIBUTE_KEY_VALUE] = (Json::Int64)intval;
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            status = dc_get_value_float (context, &floatval);
            if (status != DISIR_STATUS_OK)
                goto error;

            current[ATTRIBUTE_KEY_VALUE] = floatval;
            break;
        case DISIR_VALUE_TYPE_BOOLEAN:
            status = dc_get_value_boolean (context, &boolval);
            if  (status != DISIR_STATUS_OK)
                goto error;

            current[ATTRIBUTE_KEY_VALUE] = !!boolval;
            break;
        case DISIR_VALUE_TYPE_ENUM:
            status = dc_get_value_enum (context, &stringval, &size);
            if (status != DISIR_STATUS_OK)
                goto error;

            current[ATTRIBUTE_KEY_VALUE] = stringval;
            break;
        default:
            // HUH? Type not supported?
            disir_error_set (m_disir, "Got an unsupported disir value type");
    }

    status = serialize_attributes (context, current, DISIR_CONTEXT_DEFAULT);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    defaults.append (current);

    return status;
error:
    disir_error_set (m_disir, "Could not retrive keyval value (%s). Error: (%s)",
                               dc_value_type_string (context), disir_status_string (status));
    return status;
}


