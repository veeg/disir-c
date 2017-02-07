#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>

#include <disir/disir.h>
#include <disir/fslib/toml.h>

#include "tinytoml/toml.h"
#include "fdstream.hpp"

#define ATTRIBUTE_KEY_DISIR_CONFIG_VERSION "@DISIR_CONFIG_VERSION"
#define ATTRIBUTE_KEY_DISIR_CONFIG_VERSION_QUOTED "\"@DISIR_CONFIG_VERSION\""

// Forward declare
static enum disir_status
dio_toml_unserialize_all (struct disir_instance *instance, const char *key,
                          const toml::Value& value, struct disir_context *parent);


// STATIC
static enum disir_status
dio_toml_parse_array (struct disir_instance *instance, const char *key,
                      const toml::Value& array, struct disir_context *parent)
{
    enum disir_status status = DISIR_STATUS_OK;

    // Iterate the array, man
    for (const auto& entry : array.as<toml::Array> ())
    {
        status = dio_toml_unserialize_all (instance, key, entry, parent);
        if (status != DISIR_STATUS_OK)
        {
            break;
        }
    }

    return status;
}

// STATIC
static enum disir_status
dio_toml_parse_table (struct disir_instance *instance, const toml::Value& table,
                      struct disir_context *parent)
{
    enum disir_status status = DISIR_STATUS_OK;

    // Iterate the table, man
    for (const auto& kv : table.as<toml::Table> ())
    {
        status = dio_toml_unserialize_all (instance, kv.first.c_str (), kv.second, parent);
        if (status != DISIR_STATUS_OK)
        {
            break;
        }
    }

    return status;
}

static enum disir_status
dio_toml_unserialize_primitive (struct disir_instance *instance, const char *key,
                                const toml::Value& value, struct disir_context *parent)
{
    enum disir_status status;
    struct disir_context *context;

    status = dc_begin (parent, DISIR_CONTEXT_KEYVAL, &context);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (instance, "TOML: Cannot construct plain KEYVAL on parent: %s",
                                  disir_status_string (status));
        goto error;
    }

    status = dc_set_name (context, key, strlen (key));
    if (status != DISIR_STATUS_OK)
    {
        // QUESTION: Handle the sitation?
        disir_log_user (instance, "TOML: Name error for plain KEYVAL: %s",
                                  disir_status_string (status));
        goto error;
    }

    switch (value.type())
    {
    case toml::Value::BOOL_TYPE:
    {
        auto boolean = static_cast<uint8_t>(value.as<bool>());
        status = dc_set_value_boolean (context, boolean);
        break;
    }
    case toml::Value::DOUBLE_TYPE:
    {
        double num = value.as<double>();
        status = dc_set_value_float (context, num);
        break;
    }
    case toml::Value::INT_TYPE:
    {
        int64_t num = value.as<int64_t>();
        status = dc_set_value_integer (context, num);
        break;
    }
    case toml::Value::STRING_TYPE:
    {
        std::string str = value.as<std::string>();
        if (dc_value_type (context) == DISIR_VALUE_TYPE_ENUM)
        {
            status = dc_set_value_enum (context, str.c_str(), strlen (str.c_str()));
        }
        else
        {
            // Default to setting string type, regardless of the mold equivalent.
            // That way, if we have any error with regards to type, the context
            // will be assumed string type.
            status = dc_set_value_string (context, str.c_str(), strlen (str.c_str()));
        }
        break;
    }
    default:
    {
        disir_log_user (instance, "TOML: PROGRAMMER ERROR - unhandled type.");
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    }

    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (instance, "TOML: Value not accepted: %s",
                                  disir_status_string (status));
        goto error;
    }

    return dc_finalize (&context);
error:

    dc_destroy (&context);
    return status;
}

static enum disir_status
dio_toml_unserialize_table (struct disir_instance *instance, const char *key,
                            const toml::Value& table, struct disir_context *parent)
{
    enum disir_status status;
    struct disir_context *context;

    status = dc_begin (parent, DISIR_CONTEXT_SECTION, &context);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (instance, "TOML: Cannot construct SECTION on parent: %s",
                                  disir_status_string (status));
        goto error;
    }

    status = dc_set_name (context, key, strlen (key));
    if (status != DISIR_STATUS_OK)
    {
        // QUESTION: Handle the sitation?
        // TODO: Definately handle this situvation
        disir_log_user (instance, "TOML: Name error for SECTION: %s",
                                  disir_status_string (status));
        goto error;
    }

    // Parse the table and append it to the newly created section
    status = dio_toml_parse_table (instance, table, context);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (instance, "TOML: Sub-table failed convert: %s",
                                  disir_status_string (status));
        goto error;
    }

    return dc_finalize (&context);
error:

    dc_destroy (&context);
    return status;
}

static enum disir_status
dio_toml_unserialize_all (struct disir_instance *instance, const char *key,
                          const toml::Value& value, struct disir_context *parent)
{
    enum disir_status status = DISIR_STATUS_OK;

    switch (value.type())
    {
    case toml::Value::NULL_TYPE:
    {
        disir_log_user (instance, "TOML: Key '%s' of NULL type. Cannot reasonably handle.");
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    case toml::Value::BOOL_TYPE:
    case toml::Value::INT_TYPE:
    case toml::Value::DOUBLE_TYPE:
    case toml::Value::STRING_TYPE:
    {
        status = dio_toml_unserialize_primitive (instance, key, value, parent);
        break;
    }
    case toml::Value::TABLE_TYPE:
    {
        // Create a new section under parent, and send that into parse_table (recursive)
        status = dio_toml_unserialize_table (instance, key, value, parent);
        break;
    }
    case toml::Value::ARRAY_TYPE:
    {
        status = dio_toml_parse_array (instance, key, value, parent);
        break;
    }
    case toml::Value::TIME_TYPE:
    {
        disir_log_user (instance, "TOML: Time type not supported by Disir.");
        // TODO: Revise error code.
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    default:
    {
        disir_log_user (instance, "TOML: Type not handled: %s", value.typeAsString ());
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    }

    return status;
}

//! FSLIB API
enum disir_status
dio_toml_unserialize_config (struct disir_instance *instance, FILE *input,
                             struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *context_config;

    if (instance == NULL || input == NULL || mold == NULL || config == NULL)
    {
        // LOG debug 0
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    disir_log_user (instance, "TRACE ENTER dio_toml_unserialize_config");

    boost::fdistream file(fileno(input));
    // XXX: Check file

    // Pare the TOML formatted file and extract it into a toml::Value object
    toml::ParseResult pr = toml::parse (file);
    if (pr.valid() == false)
    {
        // LOG ERROR
        // XXX: Revise error code
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    toml::Value& root = pr.value;

    status = dc_config_begin (mold, &context_config);
    if (status != DISIR_STATUS_OK)
    {
        // LOG ERROR ?
        return status;
    }

    // Check if root contains version
    const toml::Value* version = root.findChild (ATTRIBUTE_KEY_DISIR_CONFIG_VERSION);
    if (version != nullptr)
    {
        struct semantic_version semver;

        disir_log_user (instance, "we have the config!");
        if (version->is<std::string>() == false)
        {
            // XXX: set error on CONFIG_CONTEXT
            disir_error_set (instance, "Attribute %s is of unexpected type %s."
                                       " Expecting string. Defaulting to version 1.0.0",
                                       ATTRIBUTE_KEY_DISIR_CONFIG_VERSION,
                                       version->typeAsString ());
        }
        else
        {
            status = dc_semantic_version_convert (version->as<std::string>().c_str (), &semver);
            if (status != DISIR_STATUS_OK)
            {
                // XXX: set error on CONFIG_CONTEXT
                disir_error_set (instance, "Malformed %s string '%s'.",
                                           ATTRIBUTE_KEY_DISIR_CONFIG_VERSION,
                                           version->as<std::string>().c_str ());
            }
            status = dc_set_version (context_config, &semver);
            if (status != DISIR_STATUS_OK)
            {
                // TODO. Determine how to handle this situvation.
                disir_log_user (instance, "Failed to set version on CONTEXT_CONFIG: %s",
                                          disir_status_string (status));
            }
        }

        // Remove the key so that the generic parser below doesnt middle with it.
        // XXX TOML: erase may be nested. Key must be quoted
        if (root.erase (ATTRIBUTE_KEY_DISIR_CONFIG_VERSION_QUOTED) == false)
        {
            disir_log_user (instance, "Failed to erase %s from parsed TOML object.",
                                      ATTRIBUTE_KEY_DISIR_CONFIG_VERSION);
        }
    }
    else
    {
        disir_log_user (instance, "we do not have the config!");
    }

    // Parse the entire root object into the disir config
    status = dio_toml_parse_table (instance, root, context_config);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_INVALID_CONTEXT)
    {
        disir_log_user (instance, "Fatal error in toml unserialize. Cannot provide config object");
        dc_destroy (&context_config);
        return status;
    }

    disir_log_user (instance, "TRACE EXIT dio_toml_unserialize_config");
    return dc_config_finalize (&context_config, config);
}

