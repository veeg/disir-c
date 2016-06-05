// standard includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private disir includes
#include "log.h"
#include "mqueue.h"
#include "config.h"
#include "keyval.h"
#include "default.h"
#include "context_private.h"
#include "collection.h"


//! INTERNAL API
enum disir_status
dx_default_begin (struct disir_context *parent, struct disir_context **def)
{
    enum disir_status status;
    struct disir_context *context;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // already logged
        return status;
    }
    if (def == NULL)
    {
        log_debug ("invoked with NULL def context pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // A default can only be added to a DISIR_CONTEXT_KEYVAL
    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (dc_context_type (parent->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        dx_log_context (parent, "Cannot add default to a KEYVAL whose root is not a MOLD");
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    if (dx_value_type_sanify (parent->cx_keyval->kv_value.dv_type) == DISIR_VALUE_TYPE_UNKNOWN)
    {
        dx_log_context (parent,
                        "Cannot add a default entry to a keyval"
                        " who has not yet defined a value type.");
        return DISIR_STATUS_INVALID_CONTEXT; // XXX: Revise error return code
    }

    context = dx_context_create (DISIR_CONTEXT_DEFAULT);
    if (context == NULL)
    {
        log_debug_context (parent, "failed to allocate new default context");
        return DISIR_STATUS_NO_MEMORY;
    }
    log_debug_context (parent, "created context: %p", context);

    context->cx_default = dx_default_create (context);
    if (context->cx_default == NULL)
    {
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new default instance");
        return DISIR_STATUS_NO_MEMORY;
    }
    log_debug_context (parent, "allocated new default instance: %p", context->cx_default);

    // Inherrit keyval's value type to the newly allocated default's value type.
    context->cx_default->de_value.dv_type = parent->cx_keyval->kv_value.dv_type;

    dx_context_attach (parent, context);
    *def = context;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_default_finalize (struct disir_context **default_context)
{
    enum disir_status status;
    struct disir_default *def;
    struct disir_default **queue;
    int exists;
    char buffer[32];

    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (default_context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    def = (*default_context)->cx_default;
    queue = &(*default_context)->cx_parent_context->cx_keyval->kv_default_queue;

    // TODO: Verify that the default respect the restrictions on the parent keyval

    exists = MQ_SIZE_COND (*queue,
            (dc_semantic_version_compare (&entry->de_introduced, &def->de_introduced) == 0));
    if (exists)
    {
        dx_log_context (*default_context,
                        "already contains a default entry with semantic version: %s",
                        dc_semantic_version_string (buffer, 32, &def->de_introduced));
        status = DISIR_STATUS_CONFLICTING_SEMVER;
    }
    else
    {
        MQ_ENQUEUE_CONDITIONAL (*queue, def,
            (dc_semantic_version_compare (&entry->de_introduced, &def->de_introduced) > 0));
        status = DISIR_STATUS_OK;
    }

    return status;
}

//! INTERNAL API
struct disir_default *
dx_default_create (struct disir_context *context)
{
    struct disir_default *def;

    def = calloc ( 1, sizeof (struct disir_default));
    if (def == NULL)
    {
        return NULL;
    }

    def->de_context = context;
    def->de_introduced.sv_major = 1;

    return def;
}

//! INTERNAL API
enum disir_status
dx_default_destroy (struct disir_default **def)
{
    struct disir_default *tmp;
    struct disir_default **queue;
    struct disir_context *context;

    if (def == NULL || *def == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    tmp = *def;

    if (tmp->de_value.dv_type == DISIR_VALUE_TYPE_STRING ||
        tmp->de_value.dv_size > 0)
    {
        free (tmp->de_value.dv_string);
    }

    context = tmp->de_context;
    if (context && context->cx_parent_context)
    {
        // Don't access parent context type if context is destoyed
        if (context->cx_parent_context->cx_state != CONTEXT_STATE_DESTROYED)
        {
            switch (dc_context_type (context->cx_parent_context))
            {
            case DISIR_CONTEXT_KEYVAL:
            {
                queue = &(context->cx_parent_context->cx_keyval->kv_default_queue);
                break;
            }
            default:
            {
                dx_crash_and_burn ("invoked on invalid context type (%s) - impossible",
                                   dc_context_type_string (context));
            }
            }

            MQ_REMOVE_SAFE (*queue, tmp);
        }
    }

    free (tmp);
    *def = NULL;

    return DISIR_STATUS_OK;;
}

//! PUBLIC API
enum disir_status
dc_add_default (struct disir_context *parent, const char *value,
                int32_t value_size, struct semantic_version *semver)
{
    enum disir_status status;
    enum disir_value_type type;
    char *endptr;
    long int parsed_integer;
    double parsed_double;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (value == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_INTERNAL_ERROR;

    type = dc_value_type (parent);
    if (type == DISIR_VALUE_TYPE_UNKNOWN)
    {
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    switch (type)
    {
    case DISIR_VALUE_TYPE_STRING:
    {
        status = dc_add_default_string (parent, value, value_size, semver);
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        parsed_integer = strtol (value, &endptr, 10);
        if (value == endptr)
        {
            dx_log_context (parent, "couldnt convert string to integer: '%s'", value);
            status = DISIR_STATUS_INVALID_ARGUMENT;
            break;
        }
        status = dc_add_default_integer (parent, parsed_integer, semver);
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        parsed_double = strtod (value, &endptr);
        if (value == endptr)
        {
            dx_log_context (parent, "couldn't convert string to double: '%s'", value);
            status = DISIR_STATUS_INVALID_ARGUMENT;
            break;
        }
        status = dc_add_default_float (parent, parsed_double, semver);
        break;
    }
    case DISIR_VALUE_TYPE_BOOLEAN:
    {
        // TODO: extract boolean from input string.
        //status = dc_add_default_boolean (parent, parsed_boolean, semver);
        log_fatal_context (parent, "Cannot add value type boolean: not implemented");
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    case DISIR_VALUE_TYPE_ENUM:
    {
        // TODO: determine how to handle enum
        log_fatal_context (parent, "Cannot add value type enum: not implemented");
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    }
    case DISIR_VALUE_TYPE_UNKNOWN:
    {
        // Already handled
        // No default case - let compiler warn us of unhandled cases
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_default_string (struct disir_context *parent, const char *value,
                       int32_t value_size, struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_context *def;

    def = NULL;

    status = dc_begin (parent, DISIR_CONTEXT_DEFAULT, &def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dx_value_set_string (&def->cx_default->de_value, value, value_size);
    if (status != DISIR_STATUS_OK)
    {
        // not logged to context
        goto error;
    }

    if (semver)
    {
        status = dc_add_introduced(def, *semver);
        if (status != DISIR_STATUS_OK)
        {
            // already logged
            goto error;
        }
    }

    status = dx_default_finalize (&def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged to context
        goto error;
    }

    return DISIR_STATUS_OK;
error:
    if (def)
    {
        dx_context_transfer_logwarn (parent, def);
        dc_destroy (&def);
    }
    return status;
}

//! PUBLIC API
enum disir_status
dc_add_default_integer (struct disir_context *parent, int64_t value,
                        struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_context *def;

    def = NULL;

    status = dx_default_begin (parent, &def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dx_value_set_integer (&def->cx_default->de_value, value);
    if (status != DISIR_STATUS_OK)
    {
        // not logged to context
        goto error;
    }

    if (semver)
    {
        status = dc_add_introduced(def, *semver);
        if (status != DISIR_STATUS_OK)
        {
            // already logged
            goto error;
        }
    }

    status = dx_default_finalize (&def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged to context
        goto error;
    }

    return DISIR_STATUS_OK;
error:
    if (def)
    {
        dx_context_transfer_logwarn (parent, def);
        dc_destroy (&def);
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_default_float (struct disir_context *parent, double value, struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_context *def;

    def = NULL;

    status = dx_default_begin (parent, &def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dx_value_set_float (&def->cx_default->de_value, value);
    if (status != DISIR_STATUS_OK)
    {
        // not logged to context
        goto error;
    }

    if (semver)
    {
        status = dc_add_introduced(def, *semver);
        if (status != DISIR_STATUS_OK)
        {
            // already logged
            goto error;
        }
    }

    status = dx_default_finalize (&def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged to context
        goto error;
    }

    return DISIR_STATUS_OK;
error:
    if (def)
    {
        dx_context_transfer_logwarn (parent, def);
        dc_destroy (&def);
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_default_boolean (struct disir_context *parent, uint8_t boolean,
                        struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_context *def;

    def = NULL;

    status = dx_default_begin (parent, &def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    if (boolean > 1)
    {
        boolean = 1;
    }
    status = dx_value_set_integer (&def->cx_default->de_value, boolean);
    if (status != DISIR_STATUS_OK)
    {
        // not logged to context
        goto error;
    }

    if (semver)
    {
        status = dc_add_introduced(def, *semver);
        if (status != DISIR_STATUS_OK)
        {
            // already logged
            goto error;
        }
    }

    status = dx_default_finalize (&def);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged to context
        goto error;
    }

    return DISIR_STATUS_OK;
error:
    if (def)
    {
        dx_context_transfer_logwarn (parent, def);
        dc_destroy (&def);
    }

    return status;
}


//! PUBLIC API
enum disir_status
dc_get_default (struct disir_context *context, struct semantic_version *semver,
                int32_t output_buffer_size, char *output, int32_t *output_string_size)
{
    enum disir_status status;
    struct disir_value *value;
    struct disir_context *keyval;
    struct disir_default *def;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (output_buffer_size <= 0 || output == NULL)
    {
        log_debug ("invoked with insufficient buffer size or buffer NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_DEFAULT, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        log_debug_context (context, "invoked with wrong context");
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Get the default entry associated with the input keyval
    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            // Get the mold
            keyval = context->cx_keyval->kv_mold_equiv;
        }
        else if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            keyval = context;
        }
        else
        {
            log_warn ("root context type unhandled: %s",
                      dc_context_type_string (context->cx_root_context));
            return DISIR_STATUS_INTERNAL_ERROR;
        }

        dx_default_get_active (keyval, semver, &def);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_DEFAULT)
    {
        def = context->cx_default;
    }

    value = &def->de_value;
    return dx_value_stringify (value, output_buffer_size, output, output_string_size);
}

//! PUBLIC API
enum disir_status
dc_get_default_contexts (struct disir_context *context, struct disir_collection **collection)
{
    enum disir_status status;
    struct disir_collection *col;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        dx_log_context (context, "cannot get defaults for KEYVAL not associated with a mold.");
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    col = dc_collection_create ();
    if (col == NULL)
    {
        log_debug ("failed to allocate sufficient meory for output collection");
        return DISIR_STATUS_NO_MEMORY;
    }

    //! Populate collection with each default entry on keyval
    status = DISIR_STATUS_OK;
    MQ_FOREACH (context->cx_keyval->kv_default_queue,
      {status = dc_collection_push_context (col, entry->de_context);
       if (status != DISIR_STATUS_OK) break;});

    if (status != DISIR_STATUS_OK)
    {
        log_error ("failed to populate default context collection");
        dc_collection_finished (&col);
        return status;
    }

    *collection = col;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
void
dx_default_get_active (struct disir_context *keyval, struct semantic_version *semver,
                       struct disir_default **def)
{
    struct disir_default *current;

    if (keyval->cx_keyval->kv_default_queue == NULL)
    {
        // We cant really do anything - everything is empty.
        current = NULL;
    }
    else if (semver)
    {
        current = MQ_FIND (keyval->cx_keyval->kv_default_queue,
                (dc_semantic_version_compare (&entry->de_introduced, semver) > 0));
        if (current != NULL && current->prev != MQ_TAIL (keyval->cx_keyval->kv_default_queue))
        {
            current = current->prev;
        }
        if (current == NULL)
        {
            current = MQ_TAIL (keyval->cx_keyval->kv_default_queue);
        }
    }
    else
    {
        current = MQ_TAIL (keyval->cx_keyval->kv_default_queue);
    }

    *def = current;
}

