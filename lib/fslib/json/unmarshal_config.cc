#include <disir/disir.h>
#include <json/json.h>
#include <assert.h>
#include <fstream>
#include "input.h"
#include <iostream>
#include <stdint.h>
#include "log.h"
#include "util.h" // merge into jsonIO


#define VERSION "version"

using namespace dio;

ConfigReader::ConfigReader (struct disir_instance *disir, struct disir_mold *mold)
: JsonIO (disir){
    if (disir == NULL || mold == NULL)
        throw std::invalid_argument ("");

    m_refMold = mold;
}

//! Used only when read_config_version is used
ConfigReader::ConfigReader (struct disir_instance *disir)
    : JsonIO (disir)
{
}

enum dplugin_status
ConfigReader::read_config_version (struct semantic_version *semver, const char *path)
{
    enum dplugin_status pstatus;
    enum disir_status status;

    pstatus = read_config (path, m_configRoot);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    auto version = m_configRoot[VERSION];

    if (version.isNull ())
    {
        disir_error_set (m_disir,
                "Could not read config version. ");
        // We aren't going to crash this hard, are we?
        return DPLUGIN_FATAL_ERROR;
    }

    status = dc_semantic_version_convert (version.asCString(), semver);
    if (status != DISIR_STATUS_OK)
    {
        return DPLUGIN_FATAL_ERROR;
    }
    return DPLUGIN_STATUS_OK;
}

//! PUBLIC
enum dplugin_status
ConfigReader::unmarshal (struct disir_config **config, const std::string Json)
{
    enum disir_status status;
    enum dplugin_status dstatus;
    struct disir_context *context_config = NULL;
    Json::Reader reader;

    bool success = reader.parse (Json, m_configRoot);
    if (!success)
    {
        disir_error_set (m_disir, "Parse error: %s",
                                   reader.getFormattedErrorMessages ().c_str());
        return DPLUGIN_PARSE_ERROR;
    }

    status = dc_config_begin (m_refMold, &context_config);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (m_disir, "Could not create config context from mold");
        return DPLUGIN_FATAL_ERROR;
    }

     dstatus = build_config_from_json (context_config);
     if (dstatus != DPLUGIN_STATUS_OK)
         goto error;

     status = dc_config_finalize (&context_config, config);
     if (status != DISIR_STATUS_OK)
     {
         return DPLUGIN_FATAL_ERROR;
     }

     return DPLUGIN_STATUS_OK;
error:
     if (context_config)
     {
        dc_destroy (&context_config);
     }
     return DPLUGIN_FATAL_ERROR;
}

//! PUBLIC
enum dplugin_status
ConfigReader::unmarshal (struct disir_config **config, const char *filepath)
{
    enum disir_status status;
    enum dplugin_status pstatus;
    struct disir_context *context_config = NULL;

    pstatus = read_config (filepath, m_configRoot);
    if (pstatus != DPLUGIN_STATUS_OK)
    {
        return pstatus;
    }

    status = dc_config_begin (m_refMold, &context_config);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (m_disir, "Could not create config context from mold");
        goto error;
    }

     set_config_version (context_config, m_configRoot[VERSION]);

     pstatus = build_config_from_json (context_config);
     if (pstatus != DPLUGIN_STATUS_OK)
         goto error;

     status = dc_config_finalize (&context_config, config);
     if (status != DISIR_STATUS_OK)
     {
         disir_log_user (m_disir, "could not finalize config context: %s",
                         disir_status_string (status));
         goto error;
     }

     return DPLUGIN_STATUS_OK;
error:
     if (context_config)
     {
        dc_destroy (&context_config);
     }

     return DPLUGIN_FATAL_ERROR;
}

void
ConfigReader::set_config_version (struct disir_context *context_config, Json::Value& ver)
{
    struct semantic_version semver;
    enum disir_status status;
    const char *version;

    // Absent version keyval
    if (ver.isNull () || !ver.isString ())
    {
        semver.sv_major = 1;
        semver.sv_minor = 0;
        semver.sv_patch = 0;

        append_disir_error ("Absent config version, it is set to default \"1.0.0\"");
    }
    else
    {
        version = ver.asCString ();

        status = dc_semantic_version_convert (version, &semver);
        if (status != DISIR_STATUS_OK)
        {
            semver.sv_major = 1;
            semver.sv_minor = 0;
            semver.sv_patch = 0;

            append_disir_error ("Unparseable config version, it is set to default \"1.0.0\"");
        }
    }

    dc_set_version (context_config, &semver);
}

//! PRIVATE
enum dplugin_status
ConfigReader::build_config_from_json (struct disir_context *context_config)
{
    return _unmarshal_node (context_config, m_configRoot[CONFIG]);
}

void
ConfigReader::remove_enumeration_postfix (std::string& name)
{
    std::size_t found = name.find ("@");
    if (found != std::string::npos)
    {
        name.erase (found, name.size () - found );
    }
}

//! PRIVATE
enum disir_status
ConfigReader::set_keyval (struct disir_context *parent_context,
                          std::string name, Json::Value& keyval)
{
    enum disir_status status;
    struct disir_context *context_keyval = NULL;
    Json::Value val;

    status = dc_begin (parent_context, DISIR_CONTEXT_KEYVAL, &context_keyval);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (m_disir, "could not begin context keyval: %s",
                        disir_status_string (status));
       goto error;
    }

    remove_enumeration_postfix (name);

    // if the name does not have a mold equivalent, we recieve and
    // error that it does not exist. However, we can continue.
    status = dc_set_name (context_keyval, name.c_str (), name.size ());
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_NOT_EXIST)
    {
        append_disir_error ("Could not set name on context Keyval with name (%s): %s",
                           disir_status_string (status), name.c_str ());
        goto error;
    }

    // If wrong value type is set, we can continue
    // , but set_value returns invalid context
    status = set_value (keyval, context_keyval);
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_INVALID_CONTEXT)
    {
        disir_log_user (m_disir, "something went horribly wrong: (%s) with keyname %s",
                          disir_status_string (status), name.c_str ());
        goto error;
    }

    status = dc_finalize (&context_keyval);
    if (status != DISIR_STATUS_OK &&
        status != DISIR_STATUS_INVALID_CONTEXT)
    {
        append_disir_error ("could not finalize context: %s",
                           disir_status_string (status));
        goto error;
    }

    // If context is invalid we need to explicitly
    // signal that we are done with it.
    if (status == DISIR_STATUS_INVALID_CONTEXT)
    {
        dc_putcontext (&context_keyval);
    }

    return DISIR_STATUS_OK;
error:
    if (context_keyval)
    {
        dc_destroy (&context_keyval);
    }
    return status;
}

bool
ConfigReader::value_is_section (Json::Value& node)
{
    return node.isObject ();
}

bool
ConfigReader::value_is_keyval (Json::Value& node)
{
    return !node.isArray () || !node.isObject () || !node.isNull ();
}

//! PRIVATE
enum dplugin_status
ConfigReader::_unmarshal_node (struct disir_context *parent_context, Json::Value& parent)
{
    enum disir_status status;
    enum dplugin_status dstatus;
    Json::Value child_node;
    struct disir_context *child_context = NULL;

    Json::OrderedValueIterator iter = parent.beginOrdered ();

    for (; iter != parent.endOrdered (); ++iter)
    {
        // Getting value of element
        child_node = *iter;

        auto name = iter.name ();

        if (value_is_section (child_node))
        {
            remove_enumeration_postfix (name);

            status = dc_begin (parent_context, DISIR_CONTEXT_SECTION, &child_context);
            if (status != DISIR_STATUS_OK)
            {
                // This error cannot pass, crash hard!
                disir_log_user (m_disir, "could not start DISIR_CONTEXT_SECTION");
                goto error;
            }

            status = dc_set_name (child_context, name.c_str (), name.size ());
            if (status != DISIR_STATUS_OK &&
                status != DISIR_STATUS_NOT_EXIST)
            {
                // THis is unexpected, crash hard!
                disir_log_user (m_disir, "Could not set name (%s) : %s", name.c_str (),
                                          disir_status_string (status));
                goto error;
            }

            dstatus = _unmarshal_node (child_context, child_node);
            if (dstatus != DPLUGIN_STATUS_OK)
            {
                // logged
                return dstatus;
            }

            status = dc_finalize (&child_context);
            if (status != DISIR_STATUS_OK &&
                status != DISIR_STATUS_INVALID_CONTEXT)
            {
                // Reeling us in by only loggin the error
                disir_log_user (m_disir, "Could not finalize context: %s",
                                         disir_status_string (status));
            }

            // If context is invalid we need to
            // get rid of our reference to it.
            if (status == DISIR_STATUS_INVALID_CONTEXT)
            {
                dc_putcontext (&child_context);
            }
        }
        else if (value_is_keyval (child_node))
        {
            set_keyval (parent_context, name, *iter);
        }
        else
        {
            // if object is unparsable, its name is logged
            append_disir_error ("Got unrecognized json object with name %s",
                    name.c_str ());
        }
    }

    return DPLUGIN_STATUS_OK;
error:
    if (child_context)
    {
        dc_destroy (&child_context);
    }
    return DPLUGIN_FATAL_ERROR;
}

