// External public includes
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"

//! PUBLIC API
enum disir_status
dc_printerror (dc_t *context, char *buffer, int32_t buffer_size, int32_t *bytes_written)
{
    int32_t min_bytes;

    if (context == NULL || buffer == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (context->cx_error_message == NULL ||
        context->cx_error_message_size == 0)
    {
        return DISIR_STATUS_NO_ERROR;
    }

    // XXX Null terminating character is not considered
    // FOR THE MOMENT.

    if (buffer_size < context->cx_error_message_size)
    {
        min_bytes = buffer_size;
    }
    else
    {
        min_bytes = context->cx_error_message_size;
    }

    memcpy (buffer, context->cx_error_message, min_bytes);

    return DISIR_STATUS_OK;
}

//! PUBLIC API
//! INTERNAL USAGE: Be careful not to enter a context pointer address that originate
//! from within the structure to be deleted.
enum disir_status
dc_destroy (dc_t **context)
{
    enum disir_status status;

    // Check arguments
    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_DESTROYED_CONTEXT)
    {
        // Already logged.
        return status;
    }

    // If context is destroyed, decrement and get-out-of-town
    if ((*context)->cx_state == CONTEXT_STATE_DESTROYED)
    {
        log_debug ("destroying destroyed-context( %p )", *context);
        dx_context_decref (context);
        *context = NULL;
        return DISIR_STATUS_OK;
    }

    log_debug_context (*context, "destroying (context: %p - *context: %p", context, *context);

    // Call destroy on the object pointed to by context.
    // This shall destroy the element, and every single child.
    switch (dc_context_type (*context))
    {
    case DISIR_CONTEXT_CONFIG:
        status = dx_config_destroy (&((*context)->cx_config));
        break;
    case DISIR_CONTEXT_MOLD:
        status = dx_mold_destroy (&((*context)->cx_mold));
        break;
    case DISIR_CONTEXT_DOCUMENTATION:
        status = dx_documentation_destroy (&((*context)->cx_documentation));
        break;
    case DISIR_CONTEXT_KEYVAL:
        status = dx_keyval_destroy (&((*context)->cx_keyval));
        break;
    case DISIR_CONTEXT_DEFAULT:
        status = dx_default_destroy (&((*context)->cx_default));
        break;
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_FREE_TEXT:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (*context));
    case DISIR_CONTEXT_UNKNOWN:
        // Nothing to be done. We dont know what to do!
        break;
    // No default switch handle - Let compiler warn us for unhandled case
    }

    // Set the context to destroyed
    (*context)->cx_state = CONTEXT_STATE_DESTROYED;

    // Simply decref the context - When it reaches zero, it will be dealloced
    dx_context_decref (context);

    // Take care of the users pointer. As an added service!
    *context = NULL;

    return status;
}

//! PUBLIC API
enum disir_status
dc_begin (dc_t *parent, enum disir_context_type context_type, dc_t **child)
{
    enum disir_status status;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (child == NULL)
    {
        log_debug ("invoked with child NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Disallow top-level contexts
    if (dx_context_type_is_toplevel (context_type))
    {
        dx_log_context (parent, "attempted to add top-level context %s as child",
                dx_context_type_string (context_type));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Allocate a child context using the actual allocator
    switch (dx_context_type_sanify (context_type))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
        status = dx_documentation_begin (parent, child);
        break;
    case DISIR_CONTEXT_KEYVAL:
        status = dx_keyval_begin (parent, child);
        break;
    case DISIR_CONTEXT_DEFAULT:
        status = dx_default_begin (parent, child);
        break;
    case DISIR_CONTEXT_RESTRICTION:
    case DISIR_CONTEXT_SECTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dx_context_type_string (context_type));
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_FREE_TEXT:
    case DISIR_CONTEXT_UNKNOWN:
        dx_log_context (parent, "attempted to add context of unknown type( %d )", context_type);
        status = DISIR_STATUS_INVALID_ARGUMENT;
        break;
    // No default case - Let compiler warn us on unhandled context type
    }

    // Set root context
    if (status == DISIR_STATUS_OK)
    {
        (*child)->cx_root_context = parent->cx_root_context;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_finalize (dc_t **context)
{
    enum disir_status status;

    // Check arguments
    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }

    // Dis-allow top-level contexts. They must use their own finalize method.
    if (dx_context_type_is_toplevel ((*context)->cx_type))
    {
        dx_log_context (*context, "Cannot call %s() on top-level context( %s )",
                        __FUNCTION__, dc_context_type_string (*context));
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    // Call finalize on the actual context type
    switch (dx_context_type_sanify ((*context)->cx_type))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
        status = dx_documentation_finalize (context);
        break;
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_FREE_TEXT:
        dx_crash_and_burn ("Context %s made to to switch it should never reach",
                dc_context_type_string (*context));
        break;
    case DISIR_CONTEXT_KEYVAL:
        status = dx_keyval_finalize (context);
        break;
    case DISIR_CONTEXT_DEFAULT:
        status = dx_default_finalize (context);
        break;
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (*context));
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
        log_warn ("Malwormed context object: Type value( %d )", (*context)->cx_type);
        break;
    // No default case - let compiler warn us about unhandled cases.
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_putcontext (dc_t **context)
{
    enum disir_status status;

    // Check arguments
    // Let invalid contexts into this safe heaven!
    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_DESTROYED_CONTEXT)
    {
        // Already logged.
        return status;
    }

    if ((*context)->cx_state == CONTEXT_STATE_CONSTRUCTING)
    {
        dx_log_context (*context, "You cannot put back a context in contructing mode."
                " You can either destroy it or finalize it.");
        return DISIR_STATUS_CONTEXT_IN_WRONG_STATE;
    }

    dx_context_decref (context);
    *context = NULL;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_set_name (dc_t *context, const char *name, int32_t name_size)
{
    enum disir_status status;
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
    {
        // Find the name in the mold
        // TODO: Should be sufficient to query the mold equiv of parent
        // That will solve querying sections aswell
        status = dx_set_mold_equiv (context, name, name_size);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
    }

    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        status = dx_value_set_string (&context->cx_keyval->kv_name, name, name_size);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_SECTION)
    {
        dx_crash_and_burn ("%s: Implement section", __FUNCTION__);
        //status = dx_value_set_string (&context->cx_section->se_name, value, value_size);
    }
    else
    {
        log_fatal_context (context, "slipped through guard - unsupported.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_name (dc_t *context, const char **name, int32_t *name_size)
{
    enum disir_status status;
    struct disir_value *value;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (name == NULL)
    {
        log_debug ("invoked with name NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        value = &context->cx_keyval->kv_name;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    default:
    {
        dx_crash_and_burn ("%s: %s invoked unhandled",
                           __FUNCTION__, dc_context_type_string (context));
    }
    }

    *name = value->dv_string;
    if (name_size)
    {
        *name_size = value->dv_size;
    }

    log_debug_context (context, "retrieved name: %s\n", *name);

    return DISIR_STATUS_OK;
}


// PUBLIC API
enum disir_status
dc_set_value (dc_t *context, const char *value, int32_t value_size)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_set_value_string (dc_t *context, const char *value, int32_t value_size)
{
    enum disir_status status;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (value == NULL || value_size <= 0)
    {
        dx_log_context (context, "value must be non-null and of positive length.");
        log_debug ("value: %p, value_size: %d", value_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Check capability
    if (context->CONTEXT_CAPABILITY_ADD_VALUE_STRING == 0)
    {
        dx_log_context (context, "no capability: %s",
                dx_context_capability_string (CC_ADD_VALUE_STRING));
        return DISIR_STATUS_NO_CAN_DO;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        status = dx_documentation_add_value_string (context->cx_documentation, value, value_size);
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context, "cannot set value on KEYVAL whose root is not CONFIG.");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        if (context->cx_keyval->kv_mold_equiv == NULL)
        {
            dx_log_context (context, "cannot set value on KEYVAL not associated with a mold.");
            return DISIR_STATUS_INVALID_CONTEXT;
        }
        // TODO: Validate input against mold
        status = dx_value_set_string (&context->cx_keyval->kv_value, value, value_size);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_DEFAULT:
    case DISIR_CONTEXT_FREE_TEXT:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (context));
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
    // No default case - let compiler warn us of unhandled context
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value (dc_t *context, int32_t output_buffer_size, char *output, int32_t *output_size)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged;
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_FREE_TEXT);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot get value from context type");
        return status;
    }
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context,
                            "cannot get value from KEYVAL context whose root is not CONFIG");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        status = dx_value_stringify (&context->cx_keyval->kv_value,
                                     output_buffer_size, output, output_size);
        break;
    }
    case DISIR_CONTEXT_FREE_TEXT:
    {
        status = dx_value_stringify (context->cx_value, output_buffer_size, output, output_size);
        break;
    }
    default:
    {
        status = DISIR_STATUS_INTERNAL_ERROR;
        dx_crash_and_burn ("%s - UNHANDLED_CONTEXT_TYPE: %s",
                           __FUNCTION__, dc_context_type_string (context));
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_string (dc_t *context, const char **output, int32_t *size)
{
    enum disir_status status;
    enum disir_value_type type;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = CONTEXT_TYPE_CHECK (context,
                                 DISIR_CONTEXT_KEYVAL,
                                 DISIR_CONTEXT_DOCUMENTATION,
                                 DISIR_CONTEXT_FREE_TEXT);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (output == NULL)
    {
        log_debug ("invoked with output NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dc_get_value_type (context, &type);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context (context, "cannot retrieve value type of context: %s",
                        disir_status_string (status));
        return status;
    }

    if (type != DISIR_VALUE_TYPE_STRING)
    {
        dx_log_context (context, "cannot retrieve value from context whose value is of type: %s",
                        dx_value_type_string (type));
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
        {
            dx_log_context (context, "cannot retrieve value when root context is not CONFIG");
            return DISIR_STATUS_WRONG_CONTEXT;
        }
        status = dx_value_get_string (&context->cx_keyval->kv_value, output, size);
        break;
    }
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        status = dx_value_get_string (&context->cx_documentation->dd_value, output, size);
        break;
    }
    case DISIR_CONTEXT_FREE_TEXT:
    {
        status = dx_value_get_string (context->cx_value, output, size);
        break;
    }
    default:
    {
        log_fatal_context (context, "slipped through guard - unsupported.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_value_integer (dc_t *context, int64_t *value)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_value_float (dc_t *conttext, double *value)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! INTERNAL API
enum disir_status
dx_set_mold_equiv (dc_t *context, const char *value, int32_t value_size)
{
    enum disir_status status;
    struct disir_mold *mold;
    dc_t *queried;

    if (context == NULL || value == NULL || value_size <= 0)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Should resolve within tiered in sections - mayhabs seperated by a period?

    // Holy Moley
    mold = context->cx_root_context->cx_config->cf_mold;

    if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        status = dx_element_storage_get_first (mold->mo_elements, value, &queried);
        if (status != DISIR_STATUS_OK)
        {
            // Did not find the element with that name
            log_debug ("failed to get first keyval: %s. name: %s",
                       disir_status_string (status), value);
            // XXX revise return status
            return DISIR_STATUS_INVALID_ARGUMENT;
        }
        context->cx_keyval->kv_mold_equiv = queried;
        context->cx_keyval->kv_value.dv_type = queried->cx_keyval->kv_type;
        context->cx_keyval->kv_type = queried->cx_keyval->kv_type;

        dx_context_incref (queried);
    }
    else
    {
        dx_crash_and_burn ("%s invoked with context: %s",
                           __FUNCTION__, dc_context_type_string (context));
    }

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_add_introduced (dc_t *context, struct semantic_version semver)
{
    struct semantic_version *introduced;
    enum disir_status status;
    char buffer[32];

    introduced = NULL;
    status = DISIR_STATUS_OK;

    // check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (context->CONTEXT_CAPABILITY_INTRODUCED == 0)
    {
        dx_log_context (context, "no capability to add introduced semver to context");
        return DISIR_STATUS_NO_CAN_DO;
    }

    log_debug_context (context, "adding introduced to root(%s): %s",
                       dc_context_type_string (context->cx_root_context),
                       dc_semantic_version_string (buffer, 32, &semver));

    // Update mold with highest version if root context is DISIR_CONTEXT_MOLD
    if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_MOLD)
    {
        dx_mold_update_version (context->cx_root_context->cx_mold, &semver);
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        introduced = &context->cx_documentation->dd_introduced;
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        introduced = &context->cx_default->de_introduced;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_KEYVAL:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (context));
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_FREE_TEXT:
        dx_log_context (context, "invoked %s() with capability it should not have.", __FUNCTION__);
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
        break;
    // No default handler - let compiler warn us of  unhandled context
    }

    if (introduced)
    {
        introduced->sv_major = semver.sv_major;
        introduced->sv_minor = semver.sv_minor;
        introduced->sv_patch = semver.sv_patch;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_deprecrated (dc_t *context, struct semantic_version smever)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_version (dc_t *context, struct semantic_version *semver)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, semver: %p", context, semver);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (semver == NULL)
    {
        log_debug ("invoked with semver NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_MOLD);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (dc_context_type (context) == DISIR_CONTEXT_CONFIG)
    {
        dc_semantic_version_set (semver, &context->cx_config->cf_version);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_MOLD)
    {
        dc_semantic_version_set (semver, &context->cx_mold->mo_version);
    }
    else
    {
        log_fatal_context (context, "slipped through guard - unsupported.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_set_version (dc_t *context, struct semantic_version *semver)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, semver: %p", context, semver);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (semver == NULL)
    {
        log_debug ("invoked with semver NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_MOLD);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged ?
        return status;
    }

    if (dc_context_type (context) == DISIR_CONTEXT_CONFIG)
    {
        if (dc_semantic_version_compare (&context->cx_config->cf_mold->mo_version, semver) < 0)
        {
            dx_log_context (context, "Cannot set version to CONFIG whose MOLD is lower.");
            return DISIR_STATUS_CONFLICTING_SEMVER;
        }
        dc_semantic_version_set (&context->cx_config->cf_version, semver);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_MOLD)
    {
        dc_semantic_version_set (&context->cx_mold->mo_version, semver);
    }
    else
    {
        log_fatal_context (context, "slipped through guard - unsupported.");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_get_introduced (dc_t *context, struct semantic_version *semver)
{
    struct semantic_version *introduced;
    enum disir_status status;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (semver == NULL)
    {
        log_debug ("invoked with semver NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (context->CONTEXT_CAPABILITY_INTRODUCED == 0)
    {
        dx_log_context (context, "no capability to get introduced semver from context");
        return DISIR_STATUS_NO_CAN_DO;
    }

    introduced = NULL;
    status = DISIR_STATUS_OK;

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        introduced = &context->cx_documentation->dd_introduced;
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        introduced = &context->cx_default->de_introduced;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        introduced = &context->cx_keyval->kv_introduced;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (context));
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_FREE_TEXT:
        dx_log_context (context, "invoked %s() with capability it should not have.", __FUNCTION__);
        status = DISIR_STATUS_INTERNAL_ERROR;
        break;
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
        break;
    // No default handler - let compiler warn us of  unhandled context
    }

    if (introduced != NULL)
    {
        semver->sv_major = introduced->sv_major;
        semver->sv_minor = introduced->sv_minor;
        semver->sv_patch = introduced->sv_patch;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_deprecrated (dc_t *context, struct semantic_version *semver)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_elements (dc_t *context, struct disir_collection **collection)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (collection == NULL)
    {
        log_debug ("invoked with NULL collection pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_CONFIG,
                                 DISIR_CONTEXT_MOLD, DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = DISIR_STATUS_OK;
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_MOLD:
    {
        status = dx_element_storage_get_all (context->cx_mold->mo_elements, collection);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    {
        status = dx_element_storage_get_all (context->cx_config->cf_elements, collection);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    default:
    {
        dx_crash_and_burn ("%s: %s not handled/implemented/supported",
                           __FUNCTION__, dc_context_type_string (context));
    }
    }

    return status;
}
