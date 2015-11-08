// External public includes
#include <stdlib.h>
#include <stdio.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "log.h"

//! PUBLIC API
dc_t *
dc_config_getcontext(struct disir_config *config)
{
    // Check arguments
    if (config == NULL)
    {
        // LOGWARN
        log_debug("invoked with NULL pointer");
        return NULL;
    }

    dx_context_incref(config->cf_context);

    return config->cf_context;
}

//! PUBLIC API
enum disir_status
dc_config_begin(dc_t **config)
{
    dc_t *context;

    context = NULL;

    // Disallow non-null content of passed pointer.
    if (config == NULL)
    {
        // LOGWARN
        log_debug("invoked with NULL config pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Allocate a context to be returned regardless of outcome.
    context = dx_context_create(DISIR_CONTEXT_CONFIG);
    if (context == NULL)
    {
        // LOGWARN
        log_error("failed to allocate context for config");
        return DISIR_STATUS_NO_MEMORY;
    }

    // Allocate a disir_config for this context.
    context->cx_config = dx_config_create(context);
    if (context->cx_config == NULL)
    {
        log_error("failed to allocate config for context");
        dx_context_destroy(&context);
        return DISIR_STATUS_NO_MEMORY;
    }

    *config = context;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_config_begin_supplied_schema(struct disir_schema *schema, dc_t **config)
{
    enum disir_status status;

    if (schema == NULL)
    {
        log_debug("invoked with NULL schema pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dc_config_begin(config);
    if (status != DISIR_STATUS_OK)
    {
        // Log already produced
        return status;
    }

    status = dc_config_attach_schema(*config, schema);
    if (status != DISIR_STATUS_OK)
    {
        // Log already produced
        dc_destroy(config);
        return status;
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_config_attach_schema(dc_t *config, struct disir_schema *schema)
{
    enum disir_status status;

    if (config == NULL || schema == NULL)
    {
        // LOGWARN
        log_debug("invoked with NULL pointer(s) (config: %p \t schema: %p)", config, schema);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK(config, DISIR_CONTEXT_CONFIG);
    if (status != DISIR_STATUS_OK)
    {
        // log produced by CONTEXT_TYPE_CHECK
        return status;
    }

    // Is there already a schema associated with this config?
    if (config->cx_config->cf_schema != NULL)
    {
        dx_log_context(config, "schema already exists.");
        return DISIR_STATUS_EXISTS;
    }

    config->cx_config->cf_schema = schema;
    config->CONTEXT_CAPABILITY_ADD_ENTRY = 1;
    // TODO: Add more capabilities

    return DISIR_STATUS_OK;
}

//! INTERNAL API
struct disir_config *
dx_config_create(dc_t *context)
{
    struct disir_config *config;

    config = calloc(1, sizeof(struct disir_config));
    if (config ==  NULL)
        return config;

    config->cf_context = context;

    return config;
}

//! INTERNAL API
enum disir_status
dx_config_destroy(struct disir_config **config)
{
    if (config == NULL || *config == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    // TODO: Iterate and abort all child elements of disir_config

    // Destroy single documentation, if it exists
    if ((*config)->cf_documentation)
        dc_destroy(&((*config)->cf_documentation->dd_context));

    free(*config);
    *config = NULL;

    return DISIR_STATUS_OK;
}
