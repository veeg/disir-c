
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <disir/disir.h>
#include <disir/io.h>

#include "log.h"
#include "schema.h"
#include "config.h"
#include "mqueue.h"
#include "disir_private.h"

enum disir_status
disir_register_input (struct disir *disir, const char *type, const char *description,
                      config_read config, schema_read schema)
{
    enum disir_status status;
    struct disir_input *input;

    if (type == NULL || description == NULL || config == NULL || schema == NULL)
    {
        log_debug ("invoked with NULL pointer(s). (%p %p %p %p)",
                   type, description, config, schema);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    input = calloc (1, sizeof (struct disir_input));
    if (input == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    input->type = strndup (type, DISIR_IO_TYPE_MAXLENGTH);
    if (input->type == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    input->description = strndup (description, DISIR_IO_DESCRIPTION_MAXLENGTH);
    if (input->description == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    input->config = config;
    input->schema = schema;

    MQ_PUSH (disir->dio_input_queue, input);

    return DISIR_STATUS_OK;
error:
    if (input)
    {
        free (input);
    }
    return status;
}

enum disir_status
disir_register_output (struct disir *disir, const char *type, const char *description,
                      config_write config, schema_write schema)
{
    enum disir_status status;
    struct disir_output *output;

    log_debug ("invoked");
    if (type == NULL || description == NULL || config == NULL || schema == NULL)
    {
        log_debug ("invoked with NULL pointer(s). (%p %p %p %p)",
                   type, description, config, schema);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    output = calloc (1, sizeof (struct disir_output));
    if (output == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    output->type = strndup (type, DISIR_IO_TYPE_MAXLENGTH);
    if (output->type == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    output->description = strndup (description, DISIR_IO_DESCRIPTION_MAXLENGTH);
    if (output->description == NULL)
    {
        status = DISIR_STATUS_NO_MEMORY;
        goto error;
    }

    output->config = config;
    output->schema = schema;

    MQ_PUSH (disir->dio_output_queue, output);

    return DISIR_STATUS_OK;
error:
    if (output)
    {
        free (output);
    }
    return status;
}

//! PUBLIC API
enum disir_status
disir_config_input (struct disir *disir, const char *type,
                   struct disir_config **config, const char *id)
{
    enum disir_status status;
    struct disir_input *input;

    input = MQ_FIND (disir->dio_input_queue,
                      (strncmp(entry->type, type, DISIR_IO_TYPE_MAXLENGTH) == 0));
    if (input == NULL)
    {
        log_warn ("no input type '%s'", type);
        status = DISIR_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = input->config (id, config);
    }
    return status;
}

//! PUBLIC API
enum disir_status
disir_schema_input (struct disir *disir, const char *type,
                   struct disir_schema **schema, const char *id)
{
    enum disir_status status;
    struct disir_input *input;

    input = MQ_FIND (disir->dio_input_queue,
                      (strncmp(entry->type, type, DISIR_IO_TYPE_MAXLENGTH) == 0));
    if (input== NULL)
    {
        log_warn ("no input type '%s'", type);
        status = DISIR_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = input->schema (id, schema);
    }
    return status;
}

//! PUBLIC API
enum disir_status
disir_config_output (struct disir *disir, const char *type,
                   struct disir_config *config, const char *id)
{
    enum disir_status status;
    struct disir_output *output;

    if (disir == NULL || type == NULL || config == NULL || id == NULL)
    {
        log_debug ("invoked with NULL parameters (%p %p %p %p)",
                   disir, type, config, id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    output = MQ_FIND (disir->dio_output_queue,
                      (strncmp(entry->type, type, DISIR_IO_TYPE_MAXLENGTH) == 0));
    if (output == NULL)
    {
        log_warn ("no output type '%s'", type);
        status = DISIR_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = output->config (id, config);
    }
    return status;
}

//! PUBLIC API
enum disir_status
disir_schema_output (struct disir *disir, const char *type,
                   struct disir_schema *schema, const char *id)
{
    enum disir_status status;
    struct disir_output *output;

    if (disir == NULL || type == NULL || schema == NULL || id == NULL)
    {
        log_debug ("invoked with NULL parameters (%p %p %p %p)",
                   disir, type, schema, id);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    output = MQ_FIND (disir->dio_output_queue,
                      (strncmp(entry->type, type, DISIR_IO_TYPE_MAXLENGTH) == 0));
    if (output == NULL)
    {
        log_warn ("no output type '%s'", type);
        status = DISIR_STATUS_INVALID_ARGUMENT;
    }
    else
    {
        status = output->schema (id, schema);
    }
    return status;
}

//! INTERNAL API
struct disir_output *
dx_disir_output_create (void)
{
    struct disir_output *output;

    output = calloc (1, sizeof (struct disir_output));
    if (output == NULL)
    {
        return NULL;
    }

    return output;
}

//! INTERNAL API
enum disir_status
dx_disir_output_destroy (struct disir_output **output)
{
    if (output || *output)
    {
        free (*output);
        *output  = NULL;
    }

    // TODO: REMOVE FROM PARENT

    return DISIR_STATUS_OK;
}

//! INTERNAL API
struct disir_input *
dx_disir_input_create (void)
{
    struct disir_input *input;

    input = calloc (1, sizeof (struct disir_input));
    if (input == NULL)
    {
        return NULL;
    }

    return input;
}

//! INTERNAL API
enum disir_status
dx_disir_input_destroy (struct disir_input **input)
{
    if (input || *input)
    {
        free (*input);
        *input= NULL;
    }

    // TODO: REMOVE FROM PARENT

    return DISIR_STATUS_OK;
}
