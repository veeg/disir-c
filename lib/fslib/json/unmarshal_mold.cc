// JSON private
#include "json/input.h"

// public
#include <disir/disir.h>

// standard
#include <iostream>
#include <stdarg.h>


using namespace dio;

//! Contructor
MoldReader::MoldReader (struct disir_instance *disir) : JsonIO (disir) {}

//! PUBLIC
enum dplugin_status
MoldReader::unmarshal (const char *filepath, struct disir_mold **mold)
{
    struct disir_context *context_mold = NULL;
    struct semantic_version semver;
    enum disir_status status;
    enum dplugin_status pstatus;
    std::string version;

    pstatus = read_config (filepath, m_moldRoot);

    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    pstatus = MEMBERS_CHECK (m_moldRoot, MOLD, VERSION);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    version = m_moldRoot[VERSION].asString ();
    pstatus = get_version (version, &semver);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        append_disir_error ("mold does not contain a valid version");
        goto error;
    }

    if (mold_has_documentation (m_moldRoot))
    {
        auto doc = m_moldRoot[DOCUMENTATION].asString ();
        status = dc_add_documentation (context_mold, doc.c_str (), doc.size ());
        if (status != DISIR_STATUS_OK)
        {
            append_disir_error ("Could not add mold documentation");
        }
    }

    pstatus = _unmarshal_mold (context_mold, m_moldRoot[MOLD]);
    if (pstatus != DPLUGIN_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    // If everything seemingly succeeded,
    // however if there were errors we
    if (m_errors.empty () == false)
    {
        populate_disir_with_errors ();
        return DPLUGIN_FATAL_ERROR;
    }
    return DPLUGIN_STATUS_OK;
error:
    if (context_mold)
    {
        // delete
        dc_destroy (&context_mold);
    }

    populate_disir_with_errors ();

    return DPLUGIN_FATAL_ERROR;
}

bool
MoldReader::mold_has_documentation (Json::Value& mold_root)
{
    return mold_root[DOCUMENTATION].isString ();
}

bool
MoldReader::value_is_section (Json::Value& val)
{
    return val.isObject () && val[ELEMENTS].isObject ();
}

bool
MoldReader::value_is_keyval (Json::Value& val)
{
    return val.isObject () && val[DEFAULTS].isArray ();
}

//! PRIVATE
enum dplugin_status
MoldReader::_unmarshal_mold (struct disir_context *parent_context, Json::Value& parent)
{
    Json::Value child_node;
    struct disir_context *child_context = NULL;
    enum disir_status status;
    enum dplugin_status pstatus;

    Json::OrderedValueIterator iter = parent.beginOrdered ();

    for (; iter != parent.endOrdered(); ++iter)
    {
        child_node = *iter;
        // if node is of tpe object and contains the keyval "elements", we interpret it
        // as a mold section
        if (value_is_section (child_node))
        {
            status = dc_begin (parent_context, DISIR_CONTEXT_SECTION, &child_context);
            if (status != DISIR_STATUS_OK)
                return DPLUGIN_FATAL_ERROR;

            pstatus = set_context_metadata (child_context, iter);
            if (pstatus != DPLUGIN_STATUS_OK)
                goto skip_context;

            pstatus = _unmarshal_mold (child_context, (*iter)[ELEMENTS]);
            if (pstatus != DPLUGIN_STATUS_OK)
                goto skip_context;
        }
        // if node is of type object and contains the keyval "default" we interpret it as
        // a mold keyval
        else if (value_is_keyval (child_node))
        {
            status = dc_begin (parent_context, DISIR_CONTEXT_KEYVAL, &child_context);
            if (status != DISIR_STATUS_OK)
                return DPLUGIN_FATAL_ERROR;

            pstatus = set_context_metadata (child_context, iter);
            if (pstatus != DPLUGIN_STATUS_OK)
                goto skip_context;

            pstatus = unmarshal_defaults (child_context, iter);
            if (pstatus != DPLUGIN_STATUS_OK)
                goto skip_context;
        }
        else
        {
            // We got a malformed entry, log it to disir error
            // and continue
            append_disir_error (iter,
                                "Interpreted keyval does not contain a list of defaults");
            continue;
        }

        status = dc_finalize (&child_context);
        if (status != DISIR_STATUS_OK)
        {
            return DPLUGIN_FATAL_ERROR;
        }

    // Skipping an erroneous json element
    skip_context:
        dc_destroy (&child_context);
    }

    return DPLUGIN_STATUS_OK;
}

//! json_context is of type Valueiterator because we need its name
enum dplugin_status
MoldReader::set_context_metadata (struct disir_context *context,
                                  Json::OrderedValueIterator& Jcontext)
{
    enum disir_status status;
    enum dplugin_status pstatus;
    struct semantic_version semver;
    std::string doc;
    Json::Value obj;

    obj = *Jcontext;

    pstatus = MEMBERS_CHECK (obj, INTRODUCED);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    auto name = Jcontext.name();

    status = dc_set_name (context, name.c_str(), name.size ());
    if (status != DISIR_STATUS_OK)
    {
        append_disir_error (Jcontext, "could not set name");
        return DPLUGIN_FATAL_ERROR;
    }

    // Only set type if it's not a section
    if (MEMBERS_CHECK (obj, TYPE) == DPLUGIN_STATUS_OK)
    {
        status = dc_set_value_type (context, string_to_type (obj[TYPE].asString ()));
        if (status != DISIR_STATUS_OK)
        {
            append_disir_error (Jcontext, "could not set value type %s",
                                           obj[TYPE].asString ().c_str ());
            return DPLUGIN_FATAL_ERROR;
        }
    }

    pstatus = get_version (obj[INTRODUCED].asString (), &semver);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        append_disir_error (Jcontext, "could not convert version");
        return DPLUGIN_FATAL_ERROR;
    }

    status = dc_add_introduced (context, semver);
    if (status != DISIR_STATUS_OK)
    {
        append_disir_error (Jcontext, "could not add introduced: %s",
                                       disir_status_string (status));
        return DPLUGIN_FATAL_ERROR;
    }

    doc = obj[DOCUMENTATION].asString ();
    if (doc.empty ())
    {
        // No documentation on this context exists
        return DPLUGIN_STATUS_OK;
    }

    status = dc_add_documentation (context, doc.c_str (), doc.size ());
    if (status != DISIR_STATUS_OK)
    {
        append_disir_error (Jcontext, "could not add documentation: %s",
                                       disir_status_string (status));
        return DPLUGIN_FATAL_ERROR;
    }

    return DPLUGIN_STATUS_OK;
}

enum dplugin_status
MoldReader::get_version (std::string version, struct semantic_version *semver)
{
    const char *ver;
    enum disir_status status;

    ver = version.c_str ();

    status = dc_semantic_version_convert (ver, semver);
    if (status != DISIR_STATUS_OK)
    {
        return DPLUGIN_FATAL_ERROR;
    }

    return DPLUGIN_STATUS_OK;
}

enum dplugin_status
MoldReader::unmarshal_defaults (struct disir_context *context_keyval,
                                Json::OrderedValueIterator& it)
{
    struct disir_context *context_default = NULL;
    enum disir_status status;
    enum dplugin_status pstatus;
    Json::Value defaults;

    pstatus = MEMBERS_CHECK (*it, DEFAULTS);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    defaults = (*it)[DEFAULTS];

    Json::ValueIterator iter = defaults.begin ();

    for (; iter != defaults.end (); ++iter)
    {
        status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
        if (status != DISIR_STATUS_OK)
        {
            append_disir_error (it, "dc_begin resulted in error: %s",
                                     disir_status_string (status));
            return DPLUGIN_FATAL_ERROR;
        }

        pstatus = fetch_default_data (context_default, iter);
        if (pstatus != DPLUGIN_STATUS_OK)
            goto error;

        status = dc_finalize (&context_default);
        if (status != DISIR_STATUS_OK)
        {
            append_disir_error (it, "could not finalize defaul: %s",
                                     disir_status_string (status));
            goto error;
        }
      }

    return DPLUGIN_STATUS_OK;
error:
    if (context_default)
    {
        dc_destroy (&context_default);
    }

    return DPLUGIN_FATAL_ERROR;
}

enum dplugin_status
MoldReader::fetch_default_data (struct disir_context *context_default, Json::ValueIterator& it)
{
    enum dplugin_status pstatus;
    enum disir_status status;
    struct semantic_version intro;
    Json::Value def;

    def = *it;

    pstatus = MEMBERS_CHECK (def, INTRODUCED, VALUE);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    pstatus = get_version (def[INTRODUCED].asString (), &intro);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
         append_disir_error (it, "could not fetch version: %s",
                                  it.name ().c_str ());
        return DPLUGIN_FATAL_ERROR;
    }

    status = dc_add_introduced (context_default, intro);
    if (status != DISIR_STATUS_OK)
    {
        append_disir_error (it, "could not fetch introduced: %s",
                                 disir_status_string (status));
    }

    status = set_value (def[VALUE], context_default);
    if (status != DISIR_STATUS_OK)
    {
        append_disir_error (it, "could not set value on default: %s",
                                 disir_status_string (status));
    }

    return DPLUGIN_STATUS_OK;
}

