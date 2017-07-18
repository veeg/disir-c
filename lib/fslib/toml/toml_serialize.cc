#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>

#include <disir/disir.h>
#include <disir/fslib/toml.h>

#include "tinytoml/toml.h"
#include "fdstream.hpp"


//! Forward declare STATIC API
static enum disir_status
toml_serialize_elements (struct disir_context *context, toml::Value* current);

//! STATIC API
static enum disir_status
toml_serialize_keyval (struct disir_context *context, toml::Value* current, const char *name)
{
    enum disir_status status;
    const char *value_string;
    int64_t value_integer;
    double value_float;
    uint8_t value_boolean;


    switch (dc_value_type (context))
    {
    case DISIR_VALUE_TYPE_ENUM:
    {
        status = dc_get_value_enum (context, &value_string, NULL);
        if (status == DISIR_STATUS_OK)
        {
            if (current->type () == toml::Value::Type::ARRAY_TYPE)
                current->push ((toml::Value(value_string)));
            else
                current->setChild (name, value_string);
        }
        break;
    }
    case DISIR_VALUE_TYPE_STRING:
    {
        status = dc_get_value_string (context, &value_string, NULL);
        if (status == DISIR_STATUS_OK)
        {
            if (current->type () == toml::Value::Type::ARRAY_TYPE)
                current->push ((toml::Value(value_string)));
            else
                current->setChild (name, value_string);
        }
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        status = dc_get_value_integer (context, &value_integer);
        if (status == DISIR_STATUS_OK)
        {
            if (current->type () == toml::Value::Type::ARRAY_TYPE)
                current->push ((toml::Value (value_integer)));
            else
                current->setChild (name, value_integer);
        }
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        status = dc_get_value_float (context, &value_float);
        if (status == DISIR_STATUS_OK)
        {
            if (current->type () == toml::Value::Type::ARRAY_TYPE)
                current->push ((toml::Value (value_float)));
            else
                current->setChild (name, value_float);
        }
        break;
    }
    case DISIR_VALUE_TYPE_BOOLEAN:
    {
        status = dc_get_value_boolean (context, &value_boolean);
        if (status == DISIR_STATUS_OK)
        {
            bool boolean = false;
            if (value_boolean > 0)
                boolean = true;
            if (current->type () == toml::Value::Type::ARRAY_TYPE)
                current->push ((toml::Value (boolean)));
            else
                current->setChild (name, boolean);
        }
        break;
    }
    default:
    {
        disir_log_user (NULL, "TOML: Unhandled value type of keyval: %s", dc_value_type_string (context));
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (NULL, "Failed to retrieve value from  %s keyval: %s",
                   dc_value_type_string (context), disir_status_string (status));
    }

    return status;
}

static enum disir_status
toml_serialize_inner (struct disir_context *parent, struct disir_context *context,
                      toml::Value* current, const char *name)
{
    enum disir_status status;
    struct disir_collection *key_collection;
    struct disir_context *key_element;
    toml::Value *table;

    // Get all elements from the parent
    status = dc_find_elements (parent, name, &key_collection);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (NULL, "find elements on existing name (%s) failed. Ouch. status: %s",
                   name, disir_status_string (status));
        goto out;
    }

    // Create an array storage if needed.
    if (dc_collection_size (key_collection) > 1)
    {
        current = current->setChild (name, (toml::Array()));
    }

    do
    {
        status = dc_collection_next (key_collection, &key_element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            disir_log_user (NULL, "collection next failed with non-OK status: %s",
                       disir_status_string (status));
            break;
        }

        switch (dc_context_type (context))
        {
        case DISIR_CONTEXT_KEYVAL:
        {
            status = toml_serialize_keyval (context, current, name);
            break;
        }
        case DISIR_CONTEXT_SECTION:
        {
            if (current->type() == toml::Value::Type::ARRAY_TYPE)
            {
                table = current->push ((toml::Table()));
            }
            else
            {
                table = current->setChild (name, (toml::Table()));
            }

            status = toml_serialize_elements (context, table);
            break;
        }
        default:
        {
            disir_log_user (NULL, "Unhandled child of elements: %s", dc_context_type_string (context));
            status = DISIR_STATUS_INTERNAL_ERROR;
            goto out;
        }
        }

        // Bail if the serialization goes awry
        if (status != DISIR_STATUS_OK)
        {
            goto out;
        }

        dc_putcontext (&key_element);

    } while (1);

    status = DISIR_STATUS_OK;
    // FALL-THROUGH
out:
    if (key_collection)
        dc_collection_finished (&key_collection);
    if (key_element)
        dc_putcontext (&key_element);
    return status;
}

//! STATIC API
static enum disir_status
toml_serialize_elements (struct disir_context *context, toml::Value* current)
{
    enum disir_status status;
    const char *name;
    struct disir_context *element;
    struct disir_collection *collection;

    collection = NULL;
    element = NULL;

    status = dc_get_elements (context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (NULL, "Get elements failed with status: %s", disir_status_string (status));
        goto out;
    }

    do
    {
        status = dc_collection_next (collection, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            // log error - get out of town.
            disir_log_user (NULL, "Collection returned non-OK status: %s", disir_status_string (status));
            goto out;
        }

        status = dc_get_name (element, &name, NULL);
        if (status != DISIR_STATUS_OK)
        {
            disir_log_user (NULL, "Failed to retrieve keyval name: %s", disir_status_string (status));
            goto out;
        }

        // Check if this element already exists in the current TOML value.
        // If it does, we have already added it (since there are more than one entry of it.)
        if (current->has (name))
            continue;

        status = toml_serialize_inner (context, element, current, name);
        if (status != DISIR_STATUS_OK)
            goto out;

        dc_putcontext (&element);

    } while (1);

    status = DISIR_STATUS_OK;
    // FALL-THROUGH
out:
    if (collection)
    {
        dc_collection_finished (&collection);
    }
    if (element)
    {
        dc_putcontext (&element);
    }
    return status;
}

//! FSLIB API
enum disir_status
dio_toml_serialize_config (struct disir_instance *instance,
                           struct disir_config *config, FILE *output)
{
    enum disir_status status;
    struct disir_context *context_config;

    if (config == NULL || output == NULL)
    {
        // LOG debug 0
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    disir_log_user (instance, "TRACE ENTER serialize_config");

    context_config = dc_config_getcontext (config);
    // Create a TOML document, and lets roll!!
    toml::Value root((toml::Table()));
    toml::Value* current = &root;

    // Add DISIR_CONFIG_VERSION key
    struct disir_version version;
    char buffer[512];
    status = dc_config_get_version (config, &version);
    if (status != DISIR_STATUS_OK)
    {
        // TODO: Log error
        version.sv_major = 0;
        version.sv_minor = 0;
    }
    if (dc_version_string (buffer, 512, &version) == NULL)
    {
        // TODO: Log error
        snprintf (buffer, 512, "0.0.0");
    }
    current->setChild ("@DISIR_CONFIG_VERSION", buffer);

    // Mjes
    status = toml_serialize_elements (context_config, current);

    boost::fdostream file(fileno(output));
    root.write (&file);

    disir_log_user (instance, "TRACE EXIT serialize_config");
    return DISIR_STATUS_OK;
}

