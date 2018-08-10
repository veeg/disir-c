// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

// Public disir interface
#include <disir/disir.h>

// Private
#include "context_private.h"
#include "restriction.h"
#include "log.h"
#include "mqueue.h"
#include "keyval.h"
#include "section.h"
#include "config.h"
#include "mold.h"

//! Define the size of the buffer used to name values of all restrictions
//! active in a restriction check
#define RESTRICTION_ENTRIES_BUFFER_SIZE 4096

const char *disir_restriction_strings[] = {
    "INVALID",
    "MINIMUM_ENTRIES",
    "MAXIMUM_ENTRIES",
    "ENUM",
    "RANGE",
    "NUMERIC",
    "UNKNOWN",
};


//! PUBLIC API
const char *
dc_restriction_enum_string (enum disir_restriction_type restriction)
{
    return disir_restriction_strings[restriction];
}

//! PUBLIC API
const char *
dc_restriction_context_string (struct disir_context *context)
{
    if (context == NULL || dc_context_type (context) != DISIR_CONTEXT_RESTRICTION)
    {
        return disir_restriction_strings[0];
    }

    return dc_restriction_enum_string (context->cx_restriction->re_type);
}

//! PUBLIC API
const char *
dc_restriction_group_type (enum disir_restriction_type restriction)
{
    if (restriction >= DISIR_RESTRICTION_INC_ENTRY_MIN &&
        restriction < DISIR_RESTRICTION_EXC_VALUE_ENUM)
    {
        return "INCLUSIVE";
    }
    else if (restriction >= DISIR_RESTRICTION_EXC_VALUE_ENUM &&
             restriction < DISIR_RESTRICTION_UNKNOWN)
    {
        return "EXCLUSIVE";
    }
    else
    {
        return "UNKNOWN";
    }
}

//! PUBLIC API
enum disir_restriction_type
dc_restriction_string_to_enum (const char *string)
{
    int i;

    for (i = 1; i < DISIR_RESTRICTION_UNKNOWN; i++)
    {
        if (strcmp (string, disir_restriction_strings[i]) == 0)
        {
            return (enum disir_restriction_type) i;
        }
    }

    return DISIR_RESTRICTION_UNKNOWN;
}
//
//! INTERNAL_API
enum disir_restriction_type
dx_restriction_type_sanify (enum disir_restriction_type type)
{
    if (type >= DISIR_RESTRICTION_UNKNOWN || type <= 0)
    {
        return DISIR_RESTRICTION_UNKNOWN;
    }

    return type;
}

//! INTERNAL API
//! Only called from dc_begin
enum disir_status
dx_restriction_begin (struct disir_context *parent, struct disir_context **restriction)
{
    enum disir_status status;
    struct disir_context *context;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // already logged
        return status;
    }
    if (restriction == NULL)
    {
        log_debug (0, "invoked with NULL restriction context pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // A restriction can only be added to KEYVAL and SECTION (whose top-level context is MOLD)
    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_KEYVAL,
                                         DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (dc_context_type (parent->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        dx_log_context (parent, "Cannot add restriction to a %s whose top-level is not a MOLD.",
                                dc_context_type_string (parent));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    context = dx_context_create (DISIR_CONTEXT_RESTRICTION);
    if (context == NULL)
    {
        log_debug_context (1, parent, "failed to allocate new restriction context.");
        return DISIR_STATUS_NO_MEMORY;
    }
    log_debug_context (8, parent, "created context: %p", context);

    context->cx_restriction = dx_restriction_create (context);
    if (context->cx_restriction == NULL)
    {
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new restriction instance");
        return DISIR_STATUS_NO_MEMORY;
    }
    log_debug_context (8, parent, "allocated new restriction instance: %p",
                                  context->cx_restriction);

    dx_context_attach (parent, context);
    *restriction = context;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
//! Only called from dc_finalize
enum disir_status
dx_restriction_finalize (struct disir_context *context)
{
    enum disir_status status;
    struct disir_restriction **queue;

    queue = NULL;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Validate
    status = dx_validate_context (context);

    // Insert into parent
    // queue
    dx_restriction_get_queue (context, &queue);
    if (queue && (status == DISIR_STATUS_OK || status == DISIR_STATUS_INVALID_CONTEXT))
    {
        //struct disir_restriction *q = *queue;
        // Enqueue
        MQ_ENQUEUE (*queue, context->cx_restriction);
        context->CONTEXT_STATE_IN_PARENT = 1;
    }
    else
    {
        // We where not able to insert ourselves into the parent queue
        // How do we indicate this error - if we return INVALID CONTEXT
        // and the user putcontext on it, we are leaking a reference we cannot
        // reach
        status = DISIR_STATUS_WRONG_VALUE_TYPE;
        log_error_context (context , "Unable to insert restriction type %s into parent.",
                                     dc_restriction_context_string (context));
    }

    // Return validate status
    return status;
}

//! INTERNAL API
struct disir_restriction *
dx_restriction_create (struct disir_context *context)
{
    struct disir_restriction *restriction;

    restriction = calloc ( 1, sizeof (struct disir_restriction));
    if (restriction == NULL)
    {
        return NULL;
    }

    restriction->re_context = context;
    restriction->re_introduced.sv_major = 1;
    restriction->re_deprecated.sv_major = 0;
    restriction->re_type = DISIR_RESTRICTION_UNKNOWN;

    return restriction;
}

//! INTERNAL API
enum disir_status
dx_restriction_destroy (struct disir_restriction **restriction)
{
    struct disir_restriction *tmp;
    struct disir_context *context;
    struct disir_restriction **queue;
    struct disir_documentation *doc;

    queue = NULL;

    tmp = *restriction;
    if (tmp->re_value_string)
    {
        free (tmp->re_value_string);
    }

    // Find parent context queue and remove safelt from there
    context = tmp->re_context;
    if (context && context->cx_parent_context)
    {
        if (context->cx_parent_context->CONTEXT_STATE_DESTROYED == 0)
        {
            if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_KEYVAL)
            {
                queue = &context->cx_parent_context->cx_keyval->kv_restrictions_queue;
            }
            else if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION)
            {
                queue = &context->cx_parent_context->cx_section->se_restrictions_queue;
            }
            else
            {
                dx_crash_and_burn ("invoked on invalid context type (%s) - impossible",
                                   dc_context_type_string (context));
            }

            if (queue)
            {
                MQ_REMOVE_SAFE (*queue, tmp);
            }
        }
    }

    // Remove documentation entries.
    while ((doc = MQ_POP((*restriction)->re_documentation_queue)))
    {
        context = doc->dd_context;
        dc_destroy (&context);
    }

    free (tmp);
    *restriction = NULL;

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_restriction_get_queue (struct disir_context *context, struct disir_restriction ***queue)
{
    if (context == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_KEYVAL)
    {
        *queue = &context->cx_parent_context->cx_keyval->kv_restrictions_queue;
    }
    else if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION)
    {
        *queue = &context->cx_parent_context->cx_section->se_restrictions_queue;
    }
    else
    {
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_get_restriction_type (struct disir_context *context, enum disir_restriction_type *type)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (type == NULL)
    {
        log_debug (0, "invoked with type NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    *type = context->cx_restriction->re_type;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_set_restriction_type (struct disir_context *context, enum disir_restriction_type type)
{
    enum disir_status status;
    enum disir_value_type value_type;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (context->CONTEXT_STATE_FINALIZED)
    {
        dx_context_error_set (context, "cannot change restriction type after it is finalized.");
        return DISIR_STATUS_CONTEXT_IN_WRONG_STATE;
    }

    // QUESTION: Should we allow invalid, constructing contexts by marking the context invalid
    // and letting it propagate through all the error checks=

    // Cannot add inclusive type restrictions to SECTION.
    if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION &&
        strcmp ("EXCLUSIVE", dc_restriction_group_type (type)) == 0)
    {
        dx_context_error_set (context, "cannot add exclusive restriction %s to SECTION.",
                              dc_restriction_enum_string (type));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Deny certain parent operations
    switch (type)
    {
        case DISIR_RESTRICTION_INC_ENTRY_MIN:
        case DISIR_RESTRICTION_INC_ENTRY_MAX:
        {
            // All types should be able to add this.
            // TODO: Check if there already exists one entry in parent- Deny request
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_ENUM:
        {
            if (context->cx_parent_context->cx_keyval->kv_value.dv_type != DISIR_VALUE_TYPE_ENUM)
            {
                dx_context_error_set (context, "cannot set restriction %s on %s KEYVAL.",
                                               dc_restriction_enum_string (type),
                                               dc_value_type_string (context->cx_parent_context));
                return DISIR_STATUS_WRONG_VALUE_TYPE;
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_RANGE:
        case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
        {
            value_type = context->cx_parent_context->cx_keyval->kv_value.dv_type;
            if (value_type != DISIR_VALUE_TYPE_INTEGER &&
                value_type != DISIR_VALUE_TYPE_FLOAT)
            {
                dx_context_error_set (context, "cannot set restriction %s on %s KEYVAL.",
                                               dc_restriction_enum_string (type),
                                               dc_value_type_string (context->cx_parent_context));
                return DISIR_STATUS_WRONG_VALUE_TYPE;

            }
            break;
        }
        case DISIR_RESTRICTION_UNKNOWN:
            return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Parent is valid entry to set type to.
    context->cx_restriction->re_type = type;

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_get_string (struct disir_context *context, const char **value)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (value == NULL)
    {
        log_debug (0, "invoked with value NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_ENUM:
    {
        *value = context->cx_restriction->re_value_string;
        break;
    }
    default:
    {
        dx_log_context (context, "cannot get string on restriction type %s.",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_set_string (struct disir_context *context, const char *value)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (value == NULL)
    {
        log_debug (0, "invoked with value NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_ENUM:
    {
        if (context->cx_restriction->re_value_string)
        {
            free (context->cx_restriction->re_value_string);
        }
        context->cx_restriction->re_value_string = strdup (value);
        break;
    }
    default:
    {
        dx_log_context (context, "cannot set string on restriction type %s.",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

enum disir_status
dc_restriction_set_range (struct disir_context *context, double min, double max)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_RANGE:
    {
        // Store restriction value based on keyval type (integer vs float)
        if (dc_value_type (context->cx_parent_context) == DISIR_VALUE_TYPE_INTEGER)
        {
            context->cx_restriction->re_value_min = (int64_t) min;
            context->cx_restriction->re_value_max = (int64_t) max;
        }
        else
        {
            context->cx_restriction->re_value_min = min;
            context->cx_restriction->re_value_max = max;
        }


        break;
    }
    default:
    {
        dx_log_context (context, "cannot set range on restriction type %s.",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_get_range (struct disir_context *context, double *min, double *max)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (min == NULL || max == NULL)
    {
        log_debug_context (0, context, "invoked with min (%p) and/or max (%p) NULL pointer(s).",
                                       min, max);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_RANGE:
    {
        *min = context->cx_restriction->re_value_min;
        *max = context->cx_restriction->re_value_max;
        break;
    }
    default:
    {
        dx_log_context (context, "cannot get range from restriction type %s.",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_set_numeric (struct disir_context *context, double value)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // XXX if set_numeric is used by entries_max/min, this doesnt make much sense....
    if (dc_value_type (context->cx_parent_context) == DISIR_VALUE_TYPE_INTEGER)
    {
        value = (int64_t) value;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
    {
        context->cx_restriction->re_value_numeric = value;
        break;
    }
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
    {
        context->cx_restriction->re_value_min = value;
        break;
    }
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    {
        context->cx_restriction->re_value_max = value;
        break;
    }
    default:
    {
        dx_log_context (context, "cannot set numeric on restriction type %s.",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_get_numeric (struct disir_context *context, double *value)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (value == NULL)
    {
        log_debug_context (0, context, "invoked with value NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    switch (context->cx_restriction->re_type)
    {
    case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
    {
        *value = context->cx_restriction->re_value_numeric;
        break;
    }
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
    {
        *value = context->cx_restriction->re_value_min;
        break;
    }
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    {
        *value = context->cx_restriction->re_value_max;
        break;
    }
    default:
    {
        dx_log_context (context, "cannot get numeric value from restriction type '%s'",
                                 dc_restriction_context_string (context));
        return DISIR_STATUS_WRONG_VALUE_TYPE; // TODO: Rename to _WRONG_TYPE?
    }
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_add_restriction_value_range (struct disir_context *parent, double min, double max,
                                const char *doc, struct disir_version *version,
                                struct disir_context **output)
{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), min (%f), max (%f) doc (%s), version (%p), output (%p)",
                 parent, min, max, doc, version, output);


    status = dc_begin (parent, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    status = dc_set_restriction_type (context_restriction, DISIR_RESTRICTION_EXC_VALUE_RANGE);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dc_restriction_set_range (context_restriction, min, max);
    if (status != DISIR_STATUS_OK)
    {
        // This should not happen....
        goto error;
    }

    if (version)
    {
        status = dc_add_introduced (context_restriction, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }
    if (doc)
    {
        status = dc_add_documentation (context_restriction, doc, strlen (doc));
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }

    if (output)
    {
        dx_context_incref (context_restriction);
        *output = context_restriction;
    }

    status = dc_finalize (&context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        if (output)
        {
            dx_context_decref (&context_restriction);
            *output = NULL;
        }
        goto error;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return DISIR_STATUS_OK;
error:
    dx_context_transfer_logwarn (parent, context_restriction);
    dc_destroy (&context_restriction);

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_restriction_value_numeric (struct disir_context *parent, double value, const char *doc,
                                  struct disir_version *version,
                                  struct disir_context **output)
{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), value (%f), doc (%s), version (%p), output (%p)",
                 parent, value, doc, version, output);

    status = dc_begin (parent, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    status = dc_set_restriction_type (context_restriction, DISIR_RESTRICTION_EXC_VALUE_NUMERIC);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dc_restriction_set_numeric (context_restriction, value);
    if (status != DISIR_STATUS_OK)
    {
        // This should not happen....
        goto error;
    }

    if (version)
    {
        status = dc_add_introduced (context_restriction, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }
    if (doc)
    {
        status = dc_add_documentation (context_restriction, doc, strlen (doc));
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }

    if (output)
    {
        dx_context_incref (context_restriction);
        *output = context_restriction;
    }

    status = dc_finalize (&context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        if (output)
        {
            dx_context_decref (&context_restriction);
            *output = NULL;
        }
        goto error;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return DISIR_STATUS_OK;
error:
    dx_context_transfer_logwarn (parent, context_restriction);
    dc_destroy (&context_restriction);

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_restriction_value_enum (struct disir_context *parent, const char *value, const char *doc,
                               struct disir_version *version,
                               struct disir_context **output)
{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), value (%s), doc (%s), version (%p), output (%p)",
                 parent, value, doc, version, output);

    status = dc_begin (parent, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        goto error;
    }

    status = dc_set_restriction_type (context_restriction, DISIR_RESTRICTION_EXC_VALUE_ENUM);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dc_restriction_set_string (context_restriction, value);
    if (status != DISIR_STATUS_OK)
    {
        // This should not happen....
        goto error;
    }

    if (version)
    {
        status = dc_add_introduced (context_restriction, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }
    if (doc)
    {
        status = dc_add_documentation (context_restriction, doc, strlen (doc));
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }

    if (output)
    {
        dx_context_incref (context_restriction);
        *output = context_restriction;
    }

    status = dc_finalize (&context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        if (output)
        {
            dx_context_decref (&context_restriction);
            *output = NULL;
        }
        goto error;
    }

    status = DISIR_STATUS_OK;
error:
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context_restriction);
        dc_destroy (&context_restriction);
    }

    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! STATIC API
static enum disir_status
add_restriction_entries_min_max (struct disir_context *parent, int64_t value,
                                    struct disir_version *version,
                                    enum disir_restriction_type type)

{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), value (%d), version (%p), type (%s)",
                 parent, value, version, dc_restriction_enum_string (type));

    status = dc_begin (parent, DISIR_CONTEXT_RESTRICTION, &context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    status = dc_set_restriction_type (context_restriction, type);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dc_restriction_set_numeric (context_restriction, value);
    if (status != DISIR_STATUS_OK)
    {
        // This should not happen....
        goto error;
    }

    if (version)
    {
        status = dc_add_introduced (context_restriction, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }

    status = dc_finalize (&context_restriction);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    // FALL-THROUGH
error:
    if (status != DISIR_STATUS_OK)
    {
        dx_context_transfer_logwarn (parent, context_restriction);
        dc_destroy (&context_restriction);
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_add_restriction_entries_min (struct disir_context *parent, int64_t min,
                                struct disir_version *version)
{
    return add_restriction_entries_min_max (parent, min, version, DISIR_RESTRICTION_INC_ENTRY_MIN);
}

//! PUBLIC API
enum disir_status
dc_add_restriction_entries_max (struct disir_context *parent, int64_t max,
                                struct disir_version *version)
{
    return add_restriction_entries_min_max (parent, max, version, DISIR_RESTRICTION_INC_ENTRY_MAX);
}

//! INTERNAL API
enum disir_status
dx_restriction_exclusive_value_check (struct disir_context *context, int64_t integer_value,
                                                                     double float_value,
                                                                     const char *string_value)
{
    enum disir_status status;
    struct disir_restriction **queue;
    struct disir_version *config_version;
    int exclusive_fulfilled = 0;
    int restriction_entries_inactive = 0;
    double value;

    char allowed_values[RESTRICTION_ENTRIES_BUFFER_SIZE];
    int allowed_written;

    status = DISIR_STATUS_OK;
    allowed_written = 0;

    TRACE_ENTER ("%s(%p), int (%d) float (%f)", dc_context_type_string (context), context,
                                                integer_value, float_value);

    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        return status;
    }

    switch (dc_value_type (context))
    {
    case DISIR_VALUE_TYPE_INTEGER:
    {
        value = (double) integer_value;
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        value = float_value;
        break;
    }
    case DISIR_VALUE_TYPE_ENUM:
        // Do not handle
        break;
    default:
    {
        log_fatal_context (context, "restriction check value type not handled: %s",
                                    dc_value_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    queue = &context->cx_keyval->kv_mold_equiv->cx_keyval->kv_restrictions_queue;
    config_version = &context->cx_root_context->cx_config->cf_version;

    MQ_FOREACH (*queue,
    {
        if (entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MIN ||
            entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MAX)
        {
            restriction_entries_inactive += 1;
            entry = entry->next;
            continue;
        }

        log_debug (10, "queue entry (%p), next (%p), prev (%p)", entry, entry->next, entry->prev);
        // Is restriction valid for this version of the config
        if (dc_version_compare (config_version, &entry->re_introduced) < 0)
        {
            log_debug (9, "queue entry introduced later than config. Skipping");
            restriction_entries_inactive += 1;
            // XXX MQ_FOREACH does not increment entry if continue is used
            entry = entry->next;
            continue;
        }

        // Has this restriction been deprecated?
        if ((entry->re_deprecated.sv_major != 0 || entry->re_deprecated.sv_minor != 0)
                && dc_version_compare (config_version, &entry->re_deprecated) >= 0)
        {
            log_debug(9, "queue entry deprecated. skipping");
            restriction_entries_inactive += 1;
            entry = entry->next;
            continue;
        }

        switch (entry->re_type)
        {
        case DISIR_RESTRICTION_EXC_VALUE_RANGE:
        {
            log_debug (9, "value (%f), min (%f), max (%f)", value,
                          entry->re_value_min, entry->re_value_max);

            if (dc_value_type (context) == DISIR_VALUE_TYPE_INTEGER)
            {
                allowed_written += snprintf (allowed_values + allowed_written,
                                             RESTRICTION_ENTRIES_BUFFER_SIZE - allowed_written,
                                             "[%lld, %lld], ",
                                             (long long int)entry->re_value_min,
                                             (long long int)entry->re_value_max);
            }
            else if (dc_value_type (context) == DISIR_VALUE_TYPE_FLOAT)
            {
                allowed_written += snprintf (allowed_values + allowed_written,
                                             RESTRICTION_ENTRIES_BUFFER_SIZE - allowed_written,
                                             "[%f, %f], ",
                                             entry->re_value_min, entry->re_value_max);
            }
            if (allowed_written >= RESTRICTION_ENTRIES_BUFFER_SIZE)
            {
                // Set the written property to the size of the buffer, dis-allowing any
                // further writes.
                allowed_written = RESTRICTION_ENTRIES_BUFFER_SIZE;
            }

            if (value >= entry->re_value_min && value <= entry->re_value_max)
            {
                log_debug (8, "Exclusive restriction %s(%p) fulfilled. (Value (%f) == [%f, %f])",
                              dc_restriction_enum_string (entry->re_type), entry,
                              value, entry->re_value_min, entry->re_value_max);
                exclusive_fulfilled = 1;
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
        {
            log_debug (9, "value (%f) numeric (%f)", value, entry->re_value_numeric);

            if (dc_value_type (context) == DISIR_VALUE_TYPE_INTEGER)
            {
                allowed_written += snprintf (allowed_values + allowed_written,
                                             RESTRICTION_ENTRIES_BUFFER_SIZE - allowed_written,
                                             "%lld, ", (long long int)entry->re_value_numeric);
            }
            else if (dc_value_type (context) == DISIR_VALUE_TYPE_FLOAT)
            {
                allowed_written += snprintf (allowed_values + allowed_written,
                                             RESTRICTION_ENTRIES_BUFFER_SIZE - allowed_written,
                                             "%f, ", entry->re_value_numeric);
            }
            if (allowed_written >= RESTRICTION_ENTRIES_BUFFER_SIZE)
            {
                // Set the written property to the size of the buffer, dis-allowing any
                // further writes.
                allowed_written = RESTRICTION_ENTRIES_BUFFER_SIZE;
            }

            if (value == entry->re_value_numeric)
            {
                log_debug (6, "Exclusive restriction %s(%p) fufilled. (Value (%f) == %f)",
                              dc_restriction_enum_string (entry->re_type), entry,
                              value, entry->re_value_numeric);

                exclusive_fulfilled = 1;
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_ENUM:
        {
            log_debug (9, "value (%s) enum (%s)", string_value, entry->re_value_string);

            allowed_written += snprintf (allowed_values + allowed_written,
                                         RESTRICTION_ENTRIES_BUFFER_SIZE - allowed_written,
                                         "'%s', ", entry->re_value_string);
            if (allowed_written >= RESTRICTION_ENTRIES_BUFFER_SIZE)
            {
                // Set the written property to the size of the buffer, dis-allowing any
                // further writes.
                allowed_written = RESTRICTION_ENTRIES_BUFFER_SIZE;
            }

            if (strcmp (string_value, entry->re_value_string) == 0)
            {
                // QUESTION: Check length aswell ?
                log_debug (6, "Exclusive_restriction fulfilled (Value (%s) == %s)",
                              string_value, entry->re_value_string);
                exclusive_fulfilled = 1;
            }
            break;
        }
        default:
        {
            log_warn_context (context, "unhandled exclusive restriction check: %s",
                                        dc_restriction_enum_string (entry->re_type));
        }
        }

    });

    // Strip the last ', ' from allowed_values buffer
    if (allowed_written != 0)
    {
        allowed_values[allowed_written - 2] = '\0';
    }

    // Entry did not fulfill the exclusive restrictions. Get out' here!
    if (exclusive_fulfilled == 0 && MQ_SIZE (*queue) > restriction_entries_inactive)
    {
        log_debug (4, "Exclusive restriction(s) violated."
                      " Out of range (%d) vs entires (%d)",
                      restriction_entries_inactive, MQ_SIZE (*queue));

        switch (dc_value_type (context))
        {
        case DISIR_VALUE_TYPE_INTEGER:
            dx_context_error_set (context, "No exclusive restrictions fulfilled for value %lld." \
                                  " Must be one of: %s", integer_value, allowed_values);
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            dx_context_error_set (context, "No exclusive restrictions fulfilled for value %f." \
                                  " Must be one of: %s", float_value, allowed_values);
            break;
        case DISIR_VALUE_TYPE_ENUM:
            dx_context_error_set (context, "No exclusive restrictions fulfilled for value %s." \
                                  " Must be one of: %s", string_value, allowed_values);
            break;
        default:
            // Do not handle
            break;
        }

        status = DISIR_STATUS_RESTRICTION_VIOLATED;
    }
    // Jesus christ.
    // So, if the type is ENUM, we require it to actually have some restrictions in-place.
    // If there are no value restriction, then it has per definition no legal value.
    if (exclusive_fulfilled == 0 &&
        status != DISIR_STATUS_RESTRICTION_VIOLATED &&
        dc_value_type (context) == DISIR_VALUE_TYPE_ENUM &&
        MQ_SIZE(*queue) == restriction_entries_inactive)
    {
        log_debug (4, "Missing enum restrictions. Keyval considered invalid.");
        dx_context_error_set (context, "Enum requires atleast one value restriction.");
        status = DISIR_STATUS_RESTRICTION_VIOLATED;
    }

    TRACE_EXIT ("status (%s", disir_status_string (status));
    return status;
}

//! INTERNAL API
enum disir_status
dx_restriction_entries_value (struct disir_context *context, enum disir_restriction_type type,
                              struct disir_version *version, int *output)
{
    enum disir_status status;
    struct disir_restriction **q;
    int min = -1;
    int max = -1;
    int min_closes_match = INT_MIN;
    int max_closes_match = INT_MIN;
    int diff;
    struct disir_version *element_introduced = NULL;
    struct disir_version *element_deprecated = NULL;

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_SECTION, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
        return status;

    if (type != DISIR_RESTRICTION_INC_ENTRY_MAX && type != DISIR_RESTRICTION_INC_ENTRY_MIN)
        return DISIR_STATUS_INTERNAL_ERROR;

    // Get inclusive queue from context
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_SECTION:
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            q = &context->cx_section->se_mold_equiv->cx_section->se_restrictions_queue;
            element_introduced = &context->cx_section->se_mold_equiv->cx_section->se_introduced;
            element_deprecated = &context->cx_section->se_mold_equiv->cx_section->se_deprecated;
        }
        else
        {
            q = &context->cx_section->se_restrictions_queue;
            element_introduced = &context->cx_section->se_introduced;
            element_deprecated = &context->cx_section->se_deprecated;
        }
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        struct disir_keyval *keyval_mold;
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            q = &context->cx_keyval->kv_mold_equiv->cx_keyval->kv_restrictions_queue;
            keyval_mold = context->cx_keyval->kv_mold_equiv->cx_keyval;
        }
        else
        {
            q = &context->cx_keyval->kv_restrictions_queue;
            keyval_mold = context->cx_keyval;
        }
        element_deprecated = &keyval_mold->kv_deprecated;

        // Get _lowest_ introduced for keyval
        struct disir_default **default_queue = &keyval_mold->kv_default_queue;
        MQ_FOREACH (*default_queue,
        {
            if (element_introduced == NULL)
            {
                element_introduced = &entry->de_introduced;
                entry = entry->next;
                continue;
            }
            if (dc_version_compare(&entry->de_introduced, element_introduced) < 0)
            {
                element_introduced = &entry->de_introduced;
            }

            entry = entry->next;
            continue;
        });

        break;
    }
    default:
    {
        log_fatal ("Slipped through guard - unhandled %s", dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    // Find target version from config, if not supplied
    if (version == NULL)
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            version = &context->cx_root_context->cx_config->cf_version;
        }
        else
        {
            version = &context->cx_root_context->cx_mold->mo_version;
        }
    }

    // Loop over each INCLUSIVE entry - finding max and min
    MQ_FOREACH (*q,
    {
        if (entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MIN ||
            entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MAX)
        {
            // XXX: This is severely flawed - the diff does not indicate
            // how great of a version difference
            diff = dc_version_compare  (&entry->re_introduced, version);
            // entry is newer than target version
            if (diff > 0)
            {
                entry = entry->next;
                continue;
            }

            if (entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MIN && diff > min_closes_match)
            {
                min_closes_match = diff;
                min = entry->re_value_min;
            }
            else if (entry->re_type == DISIR_RESTRICTION_INC_ENTRY_MAX && diff > max_closes_match)
            {
                max_closes_match = diff;
                max = entry->re_value_max;
            }
        }
    });

    // Set max.
    // Maximum default is 1.
    // if minimum is set, and the active maximum is lower, it will equal the minimum.
    if (max != 0 && max < min)
    {
        max = min;
    }
    if (max == -1)
    {
        max = 1;
    }

    // Mimimum entries is 1.
    if (min == -1)
    {
        min = 1;
    }

    // If the current element we are comparing was introduced after our target version,
    // we bail out
    if (element_introduced && dc_version_compare (element_introduced, version) > 0)
    {
        min = 0;
        max = -1;
    }
    // If it was deprecated on our version, or prior to our version, we bail out
    if (element_deprecated &&
        (element_deprecated->sv_major != 0 || element_deprecated->sv_minor != 0) &&
        dc_version_compare (element_deprecated, version) <= 0)
    {
        min = 0;
        max = -1;
    }

    if (type == DISIR_RESTRICTION_INC_ENTRY_MAX)
    {
        *output = max;
        log_debug(8, "maximum instances allowed: %d", max);
    }
    else if (type == DISIR_RESTRICTION_INC_ENTRY_MIN)
    {
        *output = min;
        log_debug(8, "minimum instances allowed: %d", min);
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_restriction_entries_minimum (struct disir_context *context, int *output)
{
    return dx_restriction_entries_value (context,
                                         DISIR_RESTRICTION_INC_ENTRY_MIN, NULL, output);
}

//! PUBLIC API
enum disir_status
dc_restriction_entries_maximum (struct disir_context *context, int *output)
{
    return dx_restriction_entries_value (context,
                                         DISIR_RESTRICTION_INC_ENTRY_MAX, NULL, output);
}

//! PUBLIC API
enum disir_status
dc_restriction_collection (struct disir_context *context, struct disir_collection **collection)
{
    enum disir_status status;
    struct disir_restriction **rcol;
    struct disir_collection *col;

    TRACE_ENTER ("");

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // already logged
        return status;
    }
    if (collection == NULL)
    {
        log_debug (0, "invoked will NULL collection argument");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_SECTION, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Get inclusive queue from context
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_SECTION:
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            rcol = &context->cx_section->se_mold_equiv->cx_section->se_restrictions_queue;
        }
        else
        {
            rcol = &context->cx_section->se_restrictions_queue;
        }
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            rcol = &context->cx_keyval->kv_mold_equiv->cx_keyval->kv_restrictions_queue;
        }
        else
        {
            rcol = &context->cx_keyval->kv_restrictions_queue;
        }
        break;
    }
    default:
    {
        log_fatal ("Slipped through guard - unhandled %s", dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    col = dc_collection_create ();
    if (col == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    MQ_FOREACH (*rcol,
    {
        dc_collection_push_context (col, entry->re_context);
    });

    if (dc_collection_size(col) == 0)
    {
        dc_collection_finished (&col);
        *collection = NULL;
        status = DISIR_STATUS_NOT_EXIST;
    }
    else
    {
        *collection = col;
        status = DISIR_STATUS_OK;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

