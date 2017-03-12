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

void
MoldWriter::extract_context_metadata (struct disir_context *context, Json::Value& element)
{
    char buf[500];
    const char *doc;
    enum disir_status status;
    struct semantic_version semver;

    dc_get_introduced (context, &semver);

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_SECTION:
        element[INTRODUCED] = dc_semantic_version_string (buf, 500, &semver);
        break;
    case DISIR_CONTEXT_KEYVAL:
        element[TYPE]       = dc_value_type_string (context);
        element[INTRODUCED] = dc_semantic_version_string (buf, 500, &semver);
        break;
    case DISIR_CONTEXT_MOLD:
        element[VERSION] = dc_semantic_version_string (buf, 500, &semver);
        break;
    default:
        break;
    }

    status = dc_get_documentation (context, NULL, &doc, NULL);
    if (status != DISIR_STATUS_INTERNAL_ERROR)
    {
        //! Doc is set only when present in context
        element[DOCUMENTATION] = doc;
    }
}

enum dplugin_status
MoldWriter::marshal (struct disir_mold *mold, std::string& mold_json)
{
    struct disir_context *context_mold;
    enum dplugin_status pstatus;
    Json::StyledWriter writer;
    Json::Value root;

    context_mold = dc_mold_getcontext (mold);
    if (context_mold == NULL)
        return DPLUGIN_FATAL_ERROR;

    extract_context_metadata (context_mold, root);

    pstatus = _marshal_mold_contexts (context_mold, root[MOLD]);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto end;

    mold_json = writer.writeOrdered (root);

end:
    dc_putcontext (&context_mold);
    return pstatus;
}

enum dplugin_status
MoldWriter::marshal_mold_keyval (struct disir_context *context_keyval, Json::Value& keyval)
{
    enum disir_status status;
    enum dplugin_status pstatus;
    struct disir_collection *coll;
    struct disir_context *context;
    Json::Value defaults;

    status = dc_get_default_contexts (context_keyval, &coll);
    if (status != DISIR_STATUS_OK)
    {
        disir_error_set (m_disir, "Could not acquire defualt context on keyval. Error (%s)",
                                   disir_status_string (status));
        goto error;
    }
    //! Get introduced and documentation
    extract_context_metadata (context_keyval, keyval);

    while (dc_collection_next (coll, &context)
            != DISIR_STATUS_EXHAUSTED)
    {
        pstatus = get_default (context, defaults);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            return DPLUGIN_FATAL_ERROR;
        }

        dc_putcontext (&context);
    }

    keyval[DEFAULTS] = defaults;
error:
     dc_collection_finished (&coll);

     return (status != DISIR_STATUS_OK ? DPLUGIN_FATAL_ERROR : DPLUGIN_STATUS_OK);
}

enum dplugin_status
MoldWriter::_marshal_mold_contexts (struct disir_context *parent_context, Json::Value& parent)
{
    struct disir_collection *coll;
    struct disir_context *context;
    enum disir_status status;
    enum dplugin_status pstatus;
    Json::Value child;
    const char *name;
    int32_t size;

    pstatus = DPLUGIN_STATUS_OK;

    status = dc_get_elements (parent_context, &coll);
    if (status != DISIR_STATUS_OK)
    {
        return DPLUGIN_FATAL_ERROR;
    }

    while (dc_collection_next (coll, &context)
            != DISIR_STATUS_EXHAUSTED)
    {
        switch (dc_context_type (context)) {
            case DISIR_CONTEXT_KEYVAL:
                pstatus = marshal_mold_keyval (context, child);
                if (pstatus != DPLUGIN_STATUS_OK)
                    goto end;

                break;
            case DISIR_CONTEXT_SECTION:
                extract_context_metadata (context, child);

                pstatus = _marshal_mold_contexts (context, child[ELEMENTS]);
                if (pstatus != DPLUGIN_STATUS_OK)
                    goto end;

                break;
            case DISIR_CONTEXT_RESTRICTION:
                disir_error_set (m_disir, "marshaling of restrictions is not yet implemented");
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

    return pstatus;
}

// Wraps libdisir dc_get_value to handle arbitrary value sizes
enum dplugin_status
MoldWriter::get_default (struct disir_context *context, Json::Value& defaults)
{
    enum disir_status status;
    enum disir_value_type type;
    struct semantic_version semver;
    int32_t size;
    char sembuf[500];
    double floatval;
    int64_t intval;
    const char *stringval;
    uint8_t boolval;
    std::string buf;
    Json::Value def;

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

            def[VALUE] = stringval;
            break;
        case DISIR_VALUE_TYPE_INTEGER:
            status = dc_get_value_integer (context, &intval);
            if (status != DISIR_STATUS_OK)
                goto error;

            def[VALUE] = (Json::Int64)intval;
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            status = dc_get_value_float (context, &floatval);
            if (status != DISIR_STATUS_OK)
                goto error;

            def[VALUE] = floatval;
            break;
        case DISIR_VALUE_TYPE_BOOLEAN:
            status = dc_get_value_boolean (context, &boolval);
            if  (status != DISIR_STATUS_OK)
                goto error;

            def[VALUE] = !!boolval;
            break;
        case DISIR_VALUE_TYPE_ENUM:
            // NOT IMPLEMENTED
            assert (0);
            break;
        default:
            // HUH? Type not supported?
            disir_error_set (m_disir, "Got an unsupported disir value type");
            return DPLUGIN_FATAL_ERROR;
    }

    dc_get_introduced (context, &semver);

    //! Maybe an error check would be better
    def[INTRODUCED] = dc_semantic_version_string (sembuf, 500, &semver);

    defaults.append (def);

    return DPLUGIN_STATUS_OK;
error:
    disir_error_set (m_disir, "Could not retrive keyval value (%s). Error: (%s)",
                               dc_value_type_string (context), disir_status_string (status));
    return DPLUGIN_FATAL_ERROR;
}


