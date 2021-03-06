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
#include "restriction.h"

static enum disir_status
context_get_introduced_structure (struct disir_context *context,
                                  struct disir_version **introduced)
{
    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_DOCUMENTATION:
    {
        *introduced = &context->cx_documentation->dd_introduced;
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        *introduced = &context->cx_default->de_introduced;
        break;
    }
    case DISIR_CONTEXT_SECTION:
        *introduced = &context->cx_section->se_introduced;
        break;
    case DISIR_CONTEXT_RESTRICTION:
    {
        *introduced = &context->cx_restriction->re_introduced;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        *introduced = &context->cx_mold->mo_version;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_UNKNOWN:
        return DISIR_STATUS_INTERNAL_ERROR;
    // No default handler - let compiler warn us of  unhandled context
    }

    return DISIR_STATUS_OK;
}

//! STATIC API
static enum disir_status
context_get_deprecated_structure (struct disir_context *context,
                                  struct disir_version **deprecated)
{
    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        *deprecated = &context->cx_keyval->kv_deprecated;
        break;
    }
    case DISIR_CONTEXT_SECTION:
        *deprecated = &context->cx_section->se_deprecated;
        break;
    case DISIR_CONTEXT_RESTRICTION:
    {
        *deprecated = &context->cx_restriction->re_deprecated;
        break;
    }
    default:
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

//! STATIC API
static void
context_remove_from_parent (struct disir_context **context)
{
    struct disir_element_storage *storage = NULL;
    const char *name = NULL;

    // Remove from parent, if applicable
    if ((*context)->CONTEXT_STATE_IN_PARENT == 0)
    {
        return;
    }

    switch (dc_context_type ((*context)->cx_parent_context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        storage = (*context)->cx_parent_context->cx_config->cf_elements;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        storage = (*context)->cx_parent_context->cx_section->se_elements;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        storage = (*context)->cx_parent_context->cx_mold->mo_elements;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        // The context here is a restriction - ignore
        break;
    }
    default:
    {
        log_warn ("Unhandled context type for parent element container: %s",
                  dc_context_type_string ((*context)->cx_parent_context));
    }
    }

    switch (dc_context_type (*context))
    {
    case DISIR_CONTEXT_KEYVAL:
    {
        name = (*context)->cx_keyval->kv_name.dv_string;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        name = (*context)->cx_section->se_name.dv_string;
        break;
    }
    case DISIR_CONTEXT_RESTRICTION:
    {
        // Ignore - parent has no element storage of restrictions.
        break;
    }
    default:
    {
        log_warn ("Unhandled context type to remove element from parent: %s",
                  dc_context_type_string (*context));
    }
    }

    // Remove from parent storage, if available
    if (storage && name)
    {
        log_debug(8, "Removing '%s' from parent storage", name);
        dx_element_storage_remove (storage, name, *context);
    }
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


    TRACE_ENTER ("%p", *context);

    // If context is destroyed, decrement and get-out-of-town
    if ((*context)->CONTEXT_STATE_DESTROYED)
    {
        log_debug (3, "destroying destroyed-context( %p )", *context);
        dx_context_decref (context);
        *context = NULL;
        return DISIR_STATUS_OK;
    }

    // Only applicable if CONTEXT_STATE_IN_PARENT
    context_remove_from_parent (context);

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
    case DISIR_CONTEXT_RESTRICTION:
        status = dx_restriction_destroy (&((*context)->cx_restriction));
        break;
    case DISIR_CONTEXT_UNKNOWN:
        // Nothing to be done. We dont know what to do!
        break;
    // No default switch handle - Let compiler warn us for unhandled case
    }

    // Set the context to destroyed
    (*context)->CONTEXT_STATE_DESTROYED = 1;

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

    TRACE_EXIT ("%s", disir_status_string (status));
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
        status = dx_restriction_begin (parent, child);
        break;
    case DISIR_CONTEXT_CONFIG:
    case DISIR_CONTEXT_MOLD:
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
                        __func__, dc_context_type_string (*context));
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
        status = dx_restriction_finalize (*context);
        break;
    case DISIR_CONTEXT_UNKNOWN:
        status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
        log_warn ("Malwormed context object: Type value( %d )", (*context)->cx_type);
        break;
    // No default case - let compiler warn us about unhandled cases.
    }

    // Handle context state
    if (status == DISIR_STATUS_OK ||
        status == DISIR_STATUS_INVALID_CONTEXT ||
        status == DISIR_STATUS_ELEMENTS_INVALID)
    {
        (*context)->CONTEXT_STATE_FINALIZED = 1;
        (*context)->CONTEXT_STATE_CONSTRUCTING = 0;

        if (status == DISIR_STATUS_INVALID_CONTEXT)
        {
            // Contect invalid - Mark it as so and let user handle it.
            (*context)->CONTEXT_STATE_INVALID = 1;
            // XXX: Handlse case where invalid context is not added to
            // XXX: parent element storage  (empty name)
            // XXX: Now it stands at two references with only one liable source (caller)
            // XXX: Either allow empty name to be submitted to element_storage
            // XXX: or handle it specially here.
            dx_context_incref (*context);
        }
        else
        {
            // Update mold reference, if applicable
            // XXX: Handle versions much be done more elegantly
            // XXX: Currently, we may still have dangling versions persistet to mold
            // XXX: event though they may be deleted (finalized child, destroy constructing parent)
            struct disir_version *introduced;
            struct disir_version *deprecated;
            if (context_get_introduced_structure (*context, &introduced) == DISIR_STATUS_OK)
            {
                dx_mold_update_version ((*context)->cx_root_context->cx_mold, introduced);
            }
            if (context_get_deprecated_structure (*context, &deprecated) == DISIR_STATUS_OK)
            {
                if (deprecated->sv_major != 0)
                    dx_mold_update_version ((*context)->cx_root_context->cx_mold, deprecated);
            }
            // TODO: Add similar test to deprecated as is done for introduced

            // Deprive the user of his reference.
            *context = NULL;

            // Only let API return INVALID_CONTEXT if anything is amiss
            if (status == DISIR_STATUS_ELEMENTS_INVALID)
            {
                status = DISIR_STATUS_INVALID_CONTEXT;
            }
        }
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_finalize_keep_reference (struct disir_context *context)
{
    enum disir_status status;
    struct disir_context *tmp = context;

    if (context)
    {
        dx_context_incref (context);
    }

    status = dc_finalize (&tmp);
    if (status == DISIR_STATUS_INVALID_CONTEXT && tmp)
    {
        // decref - we have an additional reference acquired above
        dx_context_decref (&context);
    }

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

    TRACE_ENTER ("*context: %p", *context);

    if ((*context)->cx_refcount == 1)
    {
        log_debug_context (4, *context, "Input context only at 1 reference."
                                        " Destroying instead of reducing refcount.");
        dc_destroy (context);
    }
    else
    {
        dx_context_decref (context);
    }
    *context = NULL;

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_context_valid (struct disir_context *context)
{
    enum disir_status status;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    status = DISIR_STATUS_OK;
    if (context->CONTEXT_STATE_INVALID)
    {
        status = DISIR_STATUS_INVALID_CONTEXT;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_set_name (struct disir_context *context, const char *name, int32_t name_size)
{
    enum disir_status status;
    enum disir_status invalid;
    int max;
    int current_entries_count;
    struct disir_collection *collection;

    collection = NULL;
    max = 0;
    invalid = DISIR_STATUS_OK;

    TRACE_ENTER ("context (%p), name (%s), name_size (%d)", context, name, name_size);

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

    // Find the name in the mold
    if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
    {
        invalid = dx_set_mold_equiv (context, name, name_size);
        if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_NOT_EXIST)
        {
            return invalid;
        }
        // If NOT EXIST, let it through and set name but mark context as invalid
        if (invalid == DISIR_STATUS_NOT_EXIST)
        {
            context->CONTEXT_STATE_INVALID = 1;
        }

        // Restriction check: Dis-allow operation if parent is finalized and max restriction exceeded
        if (invalid == DISIR_STATUS_OK && context->cx_parent_context->CONTEXT_STATE_FINALIZED)
        {
            // check if number of elements in parent exceed size of restriction
            dx_restriction_entries_value (context, DISIR_RESTRICTION_INC_ENTRY_MAX, NULL, &max);
            dc_find_elements (context->cx_parent_context, name, &collection);
            current_entries_count = dc_collection_size (collection);
            dc_collection_finished (&collection);

            log_debug (4, "Maximum restriction for entry '%s' = %d (currently at %d",
                          name, max, current_entries_count);
            if (max != 0 && max <= current_entries_count)
            {
                dx_context_error_set (context, "maximum instances of %d exceeded", max);
                return DISIR_STATUS_RESTRICTION_VIOLATED;
            }
        }
    }

    // QUESTION: Do not allow setting name on finalized context? Only if not invalid

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

    status = (status == DISIR_STATUS_OK ? invalid : status);

    // TODO:  if context is not in constructing mode (it has been finalized once)
    // remove old name from parent storage and add it under the new name.

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_name (struct disir_context *context, const char **name, int32_t *name_size)
{
    enum disir_status status;
    struct disir_value *value;

    TRACE_ENTER ("context: %p, name: %p, name_size: %d", context, name, name_size);

    value = NULL;

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
    {
        value = &context->cx_section->se_name;
        break;
    }
    default:
    {
        log_fatal ("%s context %s slipped through guard.",
                   __func__, dc_context_type_string (context));
        status = DISIR_STATUS_WRONG_CONTEXT;
    }
    }

    if (status == DISIR_STATUS_OK)
    {
        *name = value->dv_string;
        if (name_size)
        {
            *name_size = value->dv_size;
        }

        log_debug_context (6, context, "retrieved name: %s\n", *name);
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_resolve_root_name (struct disir_context *context, char **output)
{
    enum disir_status status;
    int total_size = 0;
    const char *reverse_name_index[20];
    int reverse_name_index_index[20];
    int current_name_index = -1;
    struct disir_context *current_context = NULL;
    const char *name = NULL;
    int32_t name_size;
    char *buffer;

    TRACE_ENTER ("context (%p) output (%p)", context, output);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto out;
    }
    if (output == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL, DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto out;
    }

    current_context = context;
    while (current_context != NULL)
    {
        current_name_index += 1;
        reverse_name_index_index[current_name_index] = -1;

        name_size = 0;
        status = dc_get_name (current_context, &name, &name_size);
        if (status != DISIR_STATUS_OK)
        {
            name = "undefined";
            name_size = 9;
            reverse_name_index_index[current_name_index] = 0;
        }

        // Add length of name, plus additional characters for separators and index
        //  e.g., name@12.
        total_size += name_size;
        total_size += 10;

        reverse_name_index[current_name_index] = name;
        if (reverse_name_index_index[current_name_index] == -1)
        {
            int i = -1;
            struct disir_collection *collection;
            struct disir_context *queried;

            status = dc_find_elements (current_context->cx_parent_context, name, &collection);
            if (status != DISIR_STATUS_OK)
                break;

            do
            {
                status = dc_collection_next (collection, &queried);
                if (status != DISIR_STATUS_OK && status != DISIR_STATUS_EXHAUSTED)
                    break;

                i += 1;
                if (queried == current_context)
                {
                    reverse_name_index_index[current_name_index] = i;
                    dc_putcontext (&queried);
                    status = DISIR_STATUS_OK;
                    break;
                }

                dc_putcontext (&queried);
            } while (1);

            dc_collection_finished (&collection);

            // The context is not found in the parent..
            if (status != DISIR_STATUS_OK)
            {
                break;
            }
        }

        // Resolve parent name
        current_context = current_context->cx_parent_context;
        // This is a root context - ignore it
        if (current_context->cx_parent_context == NULL)
        {
            break;
        }
    }

    // Allocate buffer for name
    buffer = malloc (total_size);
    if (buffer == NULL)
        return DISIR_STATUS_NO_MEMORY;

    int i;
    int written = 0;
    int res = 0;
    for (i = current_name_index; i >= 0; i--)
    {
        log_debug (1, "Adding reverse name: %s", reverse_name_index[i]);
        res = snprintf (buffer + written, total_size - written, "%s", reverse_name_index[i]);
        written += res;

        if (reverse_name_index_index[i] != 0)
        {
            res = snprintf (buffer + written, total_size - written,
                            "@%d", reverse_name_index_index[i]);
            written += res;
        }

        if (i != 0)
        {
            res = snprintf (buffer + written, total_size - written, ".");
            written += res;
        }
    }


    *output = buffer;
    status = DISIR_STATUS_OK;
out:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! INTERNAL API
enum disir_status
dx_get_mold_equiv (struct disir_context *context, struct disir_context **equiv)
{
    if (context == NULL || equiv == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_CONFIG:
        *equiv = context->cx_config->cf_mold->mo_context;
        break;
    case DISIR_CONTEXT_SECTION:
        *equiv = context->cx_section->se_mold_equiv;
        break;
    case DISIR_CONTEXT_KEYVAL:
        *equiv = context->cx_keyval->kv_mold_equiv;
        break;
    default:
        return DISIR_STATUS_WRONG_CONTEXT;
    }
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
        storage = parent->cx_config->cf_mold->mo_elements;
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
        log_debug (3, "failed to find name %s in parent mold equiv elements: %s",
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
        log_debug(1, "setting mold equiv with NULL pointer arguments (%p, %p, %d)",
                     context, value, value_size);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Parent section may be invalid with no mold equivalent
    // Parent config will always have a mold equivalent.
    if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_SECTION &&
        context->cx_parent_context->cx_section->se_mold_equiv == NULL)
    {
        const char *name;
        dx_value_get_string (&context->cx_parent_context->cx_section->se_name, &name, NULL);
        dx_context_error_set (context, "Parent SECTION '%s' missing mold equivalent.", name);
        return DISIR_STATUS_NOT_EXIST;
    }

    if (dc_context_type (context->cx_parent_context) == DISIR_CONTEXT_CONFIG)
    {
        storage = context->cx_parent_context->cx_config->cf_mold->mo_elements;
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
        dx_context_error_set (context, "%s missing mold equivalent entry for name '%s'.",
                                        dc_context_type_string (context), value);
        // Usually DISIR_STATUS_NOT_EXIST
        return status;
    }

    // Matching entry was not of same type.
    if (dc_context_type (context) != dc_context_type (queried))
    {
        return DISIR_STATUS_WRONG_CONTEXT;
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
dc_add_introduced (struct disir_context *context, struct disir_version *version)
{
    struct disir_version *introduced;
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
    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL,
                                          DISIR_CONTEXT_SECTION,
                                          DISIR_CONTEXT_DEFAULT,
                                          DISIR_CONTEXT_RESTRICTION,
                                          DISIR_CONTEXT_DOCUMENTATION);
    if (status != DISIR_STATUS_OK)
    {
        // Set more context-related error message
        dx_context_error_set (context, "Cannot add introduced version to %s",
                              dc_context_type_string (context));
        return status;
    }

    // QUESTION: Should we add a collision check here? If there already exists
    // an entry in parent of same context type with equal semantic version,
    // it should perhaps be deined (or permiteted with INVALID CONTEXT
    // if parent is constructing)

    status = context_get_introduced_structure (context, &introduced);
    if (status == DISIR_STATUS_OK)
    {
        introduced->sv_major = version->sv_major;
        introduced->sv_minor = version->sv_minor;

        log_debug_context (6, context, "adding introduced to root(%s): %s",
                                       dc_context_type_string (context->cx_root_context),
                                       dc_version_string (buffer, 32, version));
    }
    else if (status == DISIR_STATUS_WRONG_CONTEXT)
    {
        dx_context_error_set (context, "Cannot add introduced to %s whose top-level is %s.",
                                       dc_context_type_string (context),
                                       dc_context_type_string (context->cx_root_context));
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_deprecated (struct disir_context *context, struct disir_version *version)
{
    enum disir_status status;
    struct disir_version *deprecated;
    char buffer[32];

    TRACE_ENTER ("context (%p) version (%p)", context, version);

    deprecated = NULL;
    status = DISIR_STATUS_OK;

    // check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = CONTEXT_TYPE_CHECK (context, DISIR_CONTEXT_KEYVAL,
                                          DISIR_CONTEXT_SECTION,
                                          DISIR_CONTEXT_DEFAULT,
                                          DISIR_CONTEXT_RESTRICTION);
    if (status != DISIR_STATUS_OK)
    {
        // Set more context-related error message
        dx_context_error_set (context, "Cannot add deprecated version to %s",
                              dc_context_type_string (context));
        return status;
    }

    status = context_get_deprecated_structure (context, &deprecated);
    if (status == DISIR_STATUS_OK)
    {
        deprecated->sv_major = version->sv_major;
        deprecated->sv_minor = version->sv_minor;

        log_debug_context (6, context, "adding deprecated to root(%s): %s",
                                       dc_context_type_string (context->cx_root_context),
                                       dc_version_string (buffer, 32, version));
    }
    else if (status == DISIR_STATUS_WRONG_CONTEXT)
    {
        dx_context_error_set (context, "Cannot add deprecated to %s whose top-level is %s.",
                                       dc_context_type_string (context),
                                       dc_context_type_string (context->cx_root_context));
    }

    TRACE_EXIT ("status: %s", disir_status_string (status));
    return status;
}

//! PUBLIC API
enum disir_status
dc_get_version (struct disir_context *context, struct disir_version *version)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, version: %p", context, version);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer.");
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
        dc_version_set (version, &context->cx_config->cf_version);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_MOLD)
    {
        dc_version_set (version, &context->cx_mold->mo_version);
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
dc_set_version (struct disir_context *context, struct disir_version *version)
{
    enum disir_status status;

    TRACE_ENTER ("context: %p, version: %p", context, version);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer.");
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
        if (dc_version_compare (
                &context->cx_config->cf_mold->mo_version, version) < 0)
        {
            dx_log_context (context, "Cannot set version to CONFIG whose MOLD is lower.");
            return DISIR_STATUS_CONFLICTING_SEMVER;
        }
        dc_version_set (&context->cx_config->cf_version, version);
    }
    else if (dc_context_type (context) == DISIR_CONTEXT_MOLD)
    {
        dc_version_set (&context->cx_mold->mo_version, version);
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
dc_get_introduced (struct disir_context *context, struct disir_version *version)
{
    struct disir_version *introduced;
    enum disir_status status;

    introduced = NULL;
    status = DISIR_STATUS_OK;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = context_get_introduced_structure (context, &introduced);
    if (status == DISIR_STATUS_INTERNAL_ERROR)
    {
        status = DISIR_STATUS_WRONG_CONTEXT;
        dx_context_error_set (context, "Cannot get introduced version from %s",
                                       dc_context_type_string (context));
    }
    else if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        status = DISIR_STATUS_WRONG_CONTEXT;
        dx_context_error_set (context, "Cannot get introduced from %s whose top-level is %s.",
                                        dc_context_type_string (context),
                                        dc_context_type_string (context->cx_root_context));
    }
    else if (status == DISIR_STATUS_OK)
    {
        version->sv_major = introduced->sv_major;
        version->sv_minor = introduced->sv_minor;
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_get_deprecated (struct disir_context *context, struct disir_version *version)
{
    struct disir_version *deprecated;
    enum disir_status status;

    deprecated = NULL;
    status = DISIR_STATUS_OK;

    // Check arguments
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged.
        return status;
    }
    if (version == NULL)
    {
        log_debug (0, "invoked with version NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = context_get_deprecated_structure (context, &deprecated);
    if (status == DISIR_STATUS_INTERNAL_ERROR)
    {
        status = DISIR_STATUS_WRONG_CONTEXT;
        dx_context_error_set (context, "Cannot get deprecated version from %s",
                                       dc_context_type_string (context));
    }
    else if (dc_context_type (context->cx_root_context) != DISIR_CONTEXT_MOLD)
    {
        status = DISIR_STATUS_WRONG_CONTEXT;
        dx_context_error_set (context, "Cannot get deprecated from %s whose top-level is %s.",
                                        dc_context_type_string (context),
                                        dc_context_type_string (context->cx_root_context));
    }
    else if (status == DISIR_STATUS_OK)
    {
        version->sv_major = deprecated->sv_major;
        version->sv_minor = deprecated->sv_minor;
    }

    return status;
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
                           __func__, dc_context_type_string (context));
    }
    }

    return status;
}

//! PUBLIC API
//! TODO: Missing test
enum disir_status
dc_find_element (struct disir_context *parent, const char *name, unsigned int index,
                 struct disir_context **output)
{
    enum disir_status status;
    struct disir_collection *collection;
    uint32_t size;

    collection = NULL;

    if (output == NULL)
    {
        log_debug (0, "invoked with output NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dc_find_elements (parent, name, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    // collection size is guaranteed to be atleast 1
    size = dc_collection_size (collection);
    if ((size - 1) < index)
    {
        log_debug (5, "requested index (%d) out-of-bounds (size %d)", size, index);
        status = DISIR_STATUS_NOT_EXIST;
        goto error;
    }

    log_debug (8, "requested index (%d), collection of size (%d)", index, size);

    unsigned int i = 0;
    do
    {
        status = dc_collection_next (collection, output);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            // This should NOT happen, at all.
            log_error ("collection exhausted on iteration (%d)," \
                       " requested index (%d) out of size (%d)",
                        i, index, size);
            status = DISIR_STATUS_INTERNAL_ERROR;
            break;
        }
        if (i == index)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            // Uhmm, something bad happend
            log_error ("collection returned erroneous condition: %s",
                       disir_status_string (status));
        }
        dc_putcontext (output);
        i++;

    } while (1);

    // status is OK if output element was retrieved successfully from the collection.
    // FALL-THROUGH
error:
    if (collection)
    {
        dc_collection_finished (&collection);
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

