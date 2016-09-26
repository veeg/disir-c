// Standard includes
#include <stdlib.h>
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
dx_restriction_finalize (struct disir_context *restriction)
{
    enum disir_status status;
    struct disir_restriction *res;
    struct disir_restriction **queue;
    const char *group;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (restriction);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    res = restriction->cx_restriction;

    group = dc_restriction_group_type (res->re_type);
    if (dc_context_type (restriction->cx_parent_context) == DISIR_CONTEXT_KEYVAL)
    {
        if (strcmp ("INCLUSIVE", group) == 0)
        {
            queue = &restriction->cx_parent_context->cx_keyval->kv_restrictions_inclusive_queue;
        }
        else if (strcmp ("EXCLUSIVE", group) == 0)
        {
            queue = &restriction->cx_parent_context->cx_keyval->kv_restrictions_exclusive_queue;
        }
    }
    else if (dc_context_type (restriction->cx_parent_context) == DISIR_CONTEXT_SECTION)
    {
        if (strcmp ("INCLUSIVE", group) == 0)
        {
            queue = &restriction->cx_parent_context->cx_section->se_restrictions_inclusive_queue;
        }
        else if (strcmp ("EXCLUSIVE", group) == 0)
        {
            queue = &restriction->cx_parent_context->cx_section->se_restrictions_exclusive_queue;
        }
    }
    else
    {
        dx_log_context (restriction, "attempted finalized with unknown/unsupported type.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    // Validity check on the restriction
    switch (res->re_type)
    {
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    {
        // Verify that the introduced version does not conflict wither other entries.
        MQ_FOREACH (*queue,
        {
            if (entry->re_type == res->re_type)
            {
                if (dc_semantic_version_compare (&entry->re_introduced, &res->re_introduced) == 0)
                {
                    status = DISIR_STATUS_CONFLICTING_SEMVER;
                }
            }
        });
        break;
    }
    default:
        // No validation required
        break;
    }

    if (queue)
    {
        // Enqueue
        MQ_ENQUEUE (*queue, res);
    }
    else
    {
        log_error_context (restriction, "unknown restriction type %s",
                                        dc_restriction_context_string (restriction));
    }

    if (status == DISIR_STATUS_CONFLICTING_SEMVER)
    {
        restriction->CONTEXT_STATE_INVALID = 1;
        dx_context_error_set (restriction,
                              "introduced version conflicts with another %s restriction.",
                              dc_restriction_enum_string (res->re_type));
    }

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
    restriction->re_deprecated.sv_major = UINT_MAX;
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
    const char *group;

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
            group = dc_restriction_group_type (context->cx_restriction->re_type);
            if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_KEYVAL)
            {
                if (strcmp ("INCLUSIVE", group) == 0)
                {
                    queue = &context->cx_parent_context->cx_keyval->kv_restrictions_inclusive_queue;
                }
                else if (strcmp ("EXCLUSIVE", group) == 0)
                {
                    queue = &context->cx_parent_context->cx_keyval->kv_restrictions_exclusive_queue;
                }
            }
            else if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION)
            {
                if (strcmp ("INCLUSIVE", group) == 0)
                {
                    queue = &context->cx_parent_context->cx_section->se_restrictions_inclusive_queue;
                }
                else if (strcmp ("EXCLUSIVE", group) == 0)
                {
                    queue = &context->cx_parent_context->cx_section->se_restrictions_exclusive_queue;
                }
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
            else
            {
                log_warn_context (context, "unknown restriction group type: %s", group);
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
                                const char *doc, struct semantic_version *semver,
                                struct disir_context **output)
{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), min (%f), max (%f) doc (%s), semver (%p), output (%p)",
                 parent, min, max, doc, semver, output);


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

    if (semver)
    {
        status = dc_add_introduced (context_restriction, semver);
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
                                  struct semantic_version *semver,
                                  struct disir_context **output)
{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), value (%f), doc (%s), semver (%p), output (%p)",
                 parent, value, doc, semver, output);

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

    if (semver)
    {
        status = dc_add_introduced (context_restriction, semver);
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

//! STATIC API
static enum disir_status
add_restriction_entries_min_max (struct disir_context *parent, int64_t value,
                                    struct semantic_version *semver,
                                    enum disir_restriction_type type)

{
    enum disir_status status;
    struct disir_context *context_restriction;

    TRACE_ENTER ("parent (%p), value (%d), semver (%p), type (%s)",
                 parent, value, semver, dc_restriction_enum_string (type));

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

    if (semver)
    {
        status = dc_add_introduced (context_restriction, semver);
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
                                struct semantic_version *semver)
{
    return add_restriction_entries_min_max (parent, min, semver, DISIR_RESTRICTION_INC_ENTRY_MIN);
}

//! PUBLIC API
enum disir_status
dc_add_restriction_entries_max (struct disir_context *parent, int64_t max,
                                struct semantic_version *semver)
{
    return add_restriction_entries_min_max (parent, max, semver, DISIR_RESTRICTION_INC_ENTRY_MAX);
}

