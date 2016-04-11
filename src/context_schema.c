// external public includes
#include <stdlib.h>
#include <stdio.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "schema.h"
#include "documentation.h"
#include "mqueue.h"
#include "log.h"


//! PUBLIC API
dc_t *
dc_schema_getcontext (struct disir_schema *schema)
{
    if (schema == NULL)
    {
        log_debug ("invoked with NULL schema pointer.");
        return NULL;
    }

    dx_context_incref (schema->sc_context);

    return schema->sc_context;
}

//! PUBLIC API
enum disir_status
dc_schema_begin (dc_t **schema)
{
    dc_t *context;

    if (schema == NULL)
    {
        log_debug ("invoked with NULL schema pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    context = dx_context_create (DISIR_CONTEXT_SCHEMA);
    if (context == NULL)
    {
        log_error ("failed to allocate context for schema.");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_schema = dx_schema_create (context);
    if (context->cx_schema == NULL)
    {
        log_error ("failed to allocate schema for context.");
        dx_context_destroy (&context);
        return DISIR_STATUS_NO_MEMORY;
    }

    // Set root context to self (such that children can inherit)
    context->cx_root_context = context;

    *schema = context;
    return DISIR_STATUS_OK;
}

// PUBLIC API
enum disir_status
dc_schema_finalize (dc_t **context, struct disir_schema **schema)
{
    enum disir_status status;

    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already looged
        return status;
    }
    if (schema == NULL)
    {
        log_debug ("invoked with NULL schema pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (dx_context_type_sanify ((*context)->cx_type) != DISIR_CONTEXT_SCHEMA)
    {
        dx_log_context (*context, "Cannot call %s() on top-level context( %s )",
                        __FUNCTION__, dc_type_string (*context));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // TODO: Perform some validation?

    *schema = (*context)->cx_schema;
    (*context)->cx_state = CONTEXT_STATE_ACTIVE;
    *context = NULL;
    // We do not decref context refcount on finalize

    return DISIR_STATUS_OK;
}


//! INTERNAL API
struct disir_schema *
dx_schema_create (dc_t *context)
{
    struct disir_schema *schema;

    schema = calloc (1, sizeof (struct disir_schema));
    if (schema == NULL)
    {
        goto error;
    }


    schema->sc_context = context;
    schema->sc_elements = dx_element_storage_create ();
    if (schema->sc_elements == NULL)
    {
        goto error;
    }

    return schema;
error:
    if (schema && schema->sc_elements)
    {
        dx_element_storage_destroy (&schema->sc_elements);
    }
    if (schema)
    {
        free (schema);
    }

    return NULL;
}

//! INTERNAL API
enum disir_status
dx_schema_destroy (struct disir_schema **schema)
{
    dc_t *context;
    struct disir_documentation *doc;

    if (schema == NULL || *schema == NULL)
    {
        log_debug ("invoked with NULL schema pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Destroy every single element stored in the schema.
    dx_element_storage_destroy (&(*schema)->sc_elements);

    // Destroy the documentation associated with the schema.
    while ((doc = MQ_POP ((*schema)->sc_documentation_queue)))
    {
        context = doc->dd_context;
        dc_destroy (&context);
    }

    free (*schema);
    *schema = NULL;

    return DISIR_STATUS_OK;
}

