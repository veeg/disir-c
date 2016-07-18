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
#include "section.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"

//! PUBLIC API
enum disir_status
dc_printerror (struct disir_context *context, char *buffer,
               int32_t buffer_size, int32_t *bytes_written)
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
dc_destroy (struct disir_context **context)
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
        log_debug (3, "destroying destroyed-context( %p )", *context);
        dx_context_decref (context);
        *context = NULL;
        return DISIR_STATUS_OK;
    }

    log_debug_context (6, *context, "destroying (context: %p - *context: %p", context, *context);

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
        status = dx_section_destroy (&((*context)->cx_section));
        break;
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

    // Decref the parent ref count attained in dx_context_attach
    // Guard against decrefing ourselves (top-level contexts)
    if ((*context)->cx_parent_context && (*context)->cx_parent_context != *context)
    {
        dx_context_decref (&(*context)->cx_parent_context);
    }

    // Simply decref the context - When it reaches zero, it will be dealloced
    dx_context_decref (context);

    // Take care of the users pointer. As an added service!
    *context = NULL;

    return status;
}

//! PUBLIC API
enum disir_status
dc_begin (struct disir_context *parent, enum disir_context_type context_type,
          struct disir_context **child)
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
        log_debug (0, "invoked with child NULL pointer");
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
    case DISIR_CONTEXT_SECTION:
        status = dx_section_begin (parent, child);
        break;
    case DISIR_CONTEXT_RESTRICTION:
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
dc_finalize (struct disir_context **context)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p", context);

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
        status = dx_documentation_finalize (*context);
        break;
    case DISIR_CONTEXT_MOLD:
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_FREE_TEXT:
        dx_crash_and_burn ("Context %s made to to switch it should never reach",
                dc_context_type_string (*context));
        break;
    case DISIR_CONTEXT_KEYVAL:
        status = dx_keyval_finalize (*context);
        break;
    case DISIR_CONTEXT_DEFAULT:
        status = dx_default_finalize (*context);
        break;
    case DISIR_CONTEXT_SECTION:
        status = dx_section_finalize (*context);
        break;
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (*context));
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
        log_warn ("Malwormed context object: Type value( %d )", (*context)->cx_type);
        break;
    // No default case - let compiler warn us about unhandled cases.
    }

    // Mark the context as active and deprive the user of his reference.
    if (status == DISIR_STATUS_OK)
    {
        (*context)->cx_state = CONTEXT_STATE_ACTIVE;
        *context = NULL;
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_putcontext (struct disir_context **context)
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

    dx_context_decref (context);
    *context = NULL;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_set_name (struct disir_context *context, const char *name, int32_t name_size)
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
        status = dx_value_set_string (&context->cx_section->se_name, name, name_size);
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
dc_get_name (struct disir_context *context, const char **name, int32_t *name_size)
{
    enum disir_status status;
    struct disir_value *value;

    TRACE_ENTER ("context: %p, name: %p, name_size: %d", context, name, name_size);

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
        log_debug (0, "invoked with name NULL pointer.");
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
        value = &context->cx_section->se_name;
        break;
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

    log_debug_context (6, context, "retrieved name: %s\n", *name);

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_get_mold_equiv_type (struct disir_context *parent,
                        const char *name, enum disir_context_type *type)
{
    enum disir_status status;
    struct disir_context *queried;
    struct disir_element_storage *storage;

    if (parent == NULL || name == NULL || type == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (dc_context_type (parent->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    if (dc_context_type (parent) == DISIR_CONTEXT_CONFIG)
    {
        storage = parent->cx_config->cf_context_mold->cx_mold->mo_elements;
    }
    else if (dc_context_type (parent) == DISIR_CONTEXT_SECTION)
    {
        storage = parent->cx_section->se_mold_equiv->cx_section->se_elements;
    }
    else
    {
        dx_context_error_set (parent, "attempted to set mold_equiv on wrong context type: %s",
                              dc_context_type_string (parent));
        return DISIR_STATUS_WRONG_CONTEXT;;
    }

    status = dx_element_storage_get_first (storage, name, &queried);
    if (status != DISIR_STATUS_OK)
    {
        // Did not find the element with that name
        log_debug ("failed to find name %s in parent mold equiv elements: %s",
                   name, disir_status_string (status));
        return DISIR_STATUS_NOT_EXIST;
    }

    *type = dc_context_type (queried);

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_set_mold_equiv (struct disir_context *context, const char *value, int32_t value_size)
{
    enum disir_status status;
    struct disir_context *queried;
    struct disir_element_storage *storage;

    if (context == NULL || value == NULL || value_size <= 0)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Check that context->cx_parent_context is valid. Handle accordingly

    // TODO: add mold_equiv checks
    if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_CONFIG)
    {
        storage = context->cx_parent_context->cx_config->cf_context_mold->cx_mold->mo_elements;
    }
    else if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION)
    {
        storage = context->cx_parent_context->cx_section->se_mold_equiv->cx_section->se_elements;
    }
    else
    {
        dx_context_error_set (context, "attempted to set mold_equiv on wrong context type: %s",
                              dc_context_type_string (context));
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = dx_element_storage_get_first (storage, value, &queried);
    if (status != DISIR_STATUS_OK)
    {
        // Did not find the element with that name
        log_debug (3, "failed to find name %s in parent mold equiv elements: %s",
                   value, disir_status_string (status));
        return status;
    }

    if (dc_context_type (context) == DISIR_CONTEXT_SECTION)
    {
        context->cx_section->se_mold_equiv = queried;
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
    {
        context->cx_keyval->kv_mold_equiv = queried;
        context->cx_keyval->kv_value.dv_type = queried->cx_keyval->kv_value.dv_type;
    }

    dx_context_incref (queried);

    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_add_introduced (struct disir_context *context, struct semantic_version semver)
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

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL,
                                          DISIR_CONTEXT_DEFAULT,
                                          DISIR_CONTEXT_DOCUMENTATION);
    if (status != DISIR_STATUS_OK)
    {
        // Set more context-related error message
        dx_context_error_set (context, "Cannot add introduced version to %s",
                              dc_context_type_string (context));
        return status;
    }

    log_debug_context (6, context, "adding introduced to root(%s): %s",
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
    case DISIR_CONTEXT_KEYVAL:
    {
        introduced = &context->cx_keyval->kv_introduced;
        break;
    }
    case DISIR_CONTEXT_SECTION:
        introduced = &context->cx_section->se_introduced;
        break;
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
dc_add_deprecrated (struct disir_context *context, struct semantic_version smever)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_version (struct disir_context *context, struct semantic_version *semver)
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
        log_debug (0, "invoked with semver NULL pointer.");
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
dc_set_version (struct disir_context *context, struct semantic_version *semver)
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
        log_debug (0, "invoked with semver NULL pointer.");
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
        // TODO: Verify that the mold context is still valid. Extract into variable
        if (dc_semantic_version_compare (
                &context->cx_config->cf_context_mold->cx_mold->mo_version, semver) < 0)
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
dc_get_introduced (struct disir_context *context, struct semantic_version *semver)
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
        log_debug (0, "invoked with semver NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL,
                                          DISIR_CONTEXT_SECTION,
                                          DISIR_CONTEXT_DEFAULT,
                                          DISIR_CONTEXT_MOLD,
                                          DISIR_CONTEXT_CONFIG,
                                          DISIR_CONTEXT_DOCUMENTATION);
    if (status != DISIR_STATUS_OK)
    {
        // Set more context-related error message
        dx_context_error_set (context, "Cannot get introduced version from %s",
                              dc_context_type_string (context));
        return status;
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
    case DISIR_CONTEXT_CONFIG:
    {
        introduced = &context->cx_config->cf_version;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        introduced = &context->cx_mold->mo_version;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        introduced = &context->cx_section->se_introduced;
        break;
    }
    case DISIR_CONTEXT_RESTRICTION:
        dx_crash_and_burn ("%s - UNHANDLED CONTEXT TYPE: %s",
                __FUNCTION__, dc_context_type_string (context));
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
dc_get_deprecrated (struct disir_context *context, struct semantic_version *semver)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PUBLIC API
enum disir_status
dc_get_elements (struct disir_context *context, struct disir_collection **collection)
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
        log_debug (0, "invoked with NULL collection pointer.");
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
        status = dx_element_storage_get_all (context->cx_section->se_elements, collection);
        break;
    default:
    {
        dx_crash_and_burn ("%s: %s not handled/implemented/supported",
                           __FUNCTION__, dc_context_type_string (context));
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_find_elements (struct disir_context *context,
                  const char *name, struct disir_collection **collection)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, name: %s, collection: %p", context, name, collection);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_SECTION,
                                          DISIR_CONTEXT_MOLD,
                                          DISIR_CONTEXT_CONFIG);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
    if (collection == NULL)
    {
        log_debug (0, "invoked with NULL collection pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;
    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_MOLD:
    {
        status = dx_element_storage_get (context->cx_mold->mo_elements, name, collection);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    {
        status = dx_element_storage_get (context->cx_config->cf_elements, name, collection);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        status = dx_element_storage_get (context->cx_section->se_elements, name, collection);
        break;
    }
    default:
    {
        status = DISIR_STATUS_INTERNAL_ERROR;
        dx_context_error_set (context, "Invalid operation for context '%s'"
                                       " (slipped through internal guard)",
                                       dc_context_type_string (context));
    }
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

