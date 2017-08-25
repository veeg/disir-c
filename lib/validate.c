// External public includes
#include <stdlib.h>
#include <stdio.h>

// Public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// Private
#include "context_private.h"
#include "config.h"
#include "section.h"
#include "keyval.h"
#include "mold.h"
#include "mqueue.h"
#include "log.h"
#include "element_storage.h"
#include "restriction.h"


//! STATIC API
//!
//! Validate the children of context if they fulfill inclusive restrictions
//! within context.
//!
//! \return DISIR_STATUS_INTERNAL_ERROR if invoked with non-element bearing context.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if this context does not fulfill min/max restrictions
//!         of its children.
//! \return DISIR_STATUS_OK on success.
//!
static enum disir_status
validate_inclusive_restrictions (struct disir_context *context)
{
    enum disir_status status ;
    enum disir_status invalid;
    struct disir_collection *mold_collection;
    struct disir_collection *config_elements;
    const char *name;
    struct disir_context *element;
    int max;
    int min;
    int size;

    min = max = 0;

    invalid = DISIR_STATUS_OK;
    element = NULL;
    mold_collection = NULL;
    name = NULL;

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_CONFIG:
        status = dc_get_elements (context->cx_config->cf_mold->mo_context, &mold_collection);
        break;
    case DISIR_CONTEXT_SECTION:
        status = dc_get_elements (context->cx_section->se_mold_equiv, &mold_collection);
        break;
    default:
        log_fatal ("Invoked internal validate with incorrect context %s.",
                   dc_context_type_string (context));
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    if (status != DISIR_STATUS_OK)
    {
        log_error ("cannot get mold equivalent entries for %s.", dc_context_type_string (context));
        return status;
    }

    do
    {
        if (element)
        {
            dc_putcontext (&element);
        }

        status = dc_collection_next (mold_collection, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }

        if (status != DISIR_STATUS_OK)
            break;

        // Query how many elements in config there are of element.name
        dc_get_name (element, &name, NULL);
        status = dc_find_elements (context, name, &config_elements);
        if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
        {
            break;
        }
        if (status == DISIR_STATUS_OK)
        {
            size = dc_collection_size (config_elements);
            dc_collection_finished (&config_elements);
        }
        else
        {
            // No entries
            size = 0;
        }

        // find maximum/minimum number required.
        dx_restriction_entries_value (element, DISIR_RESTRICTION_INC_ENTRY_MIN,
                &(context)->cx_config->cf_version, &min);
        dx_restriction_entries_value (element, DISIR_RESTRICTION_INC_ENTRY_MAX,
                &(context)->cx_config->cf_version, &max);


        // Minimum restriction not fufilled.
        if (size < min)
        {
            context->CONTEXT_STATE_INVALID = 1;
            // TODO: Add complete resolved name
            dx_log_context (context,
                           "%s did not fulfill minumum required entities (minimum: %d, actual: %d)",
                           name, min, size);

            log_debug (2, "violated minimum restriction (count %d vx min %d)", size, min);
            invalid = DISIR_STATUS_RESTRICTION_VIOLATED;
        }

        // Maximum restriction not fufilled
        if (size > max && max != 0)
        {
            context->CONTEXT_STATE_INVALID = 1;
            // TODO: Add complete resolved name
            dx_log_context (context,
                           "%s exceeded maximum allowed entities (maximum: %d, actual: %d)",
                           name, max, size);
            log_debug (2, "violated maximum restriction (count %d vx max %d)", size, max);
            invalid = DISIR_STATUS_RESTRICTION_VIOLATED;
        }
    } while (1);
    if (mold_collection)
    {
        dc_collection_finished (&mold_collection);
    }

    return (status == DISIR_STATUS_OK ? invalid : status);
}

//! STATIC API
//!
//! \return DISIR_STATUS_INTERNAL_ERROR for non-handled DISIR_VALUE_TYPE
//! \return status of dx_restriction_exclusive_value_check
//!
static enum disir_status
validate_exclusive_restrictions (struct disir_context *context)
{
    enum disir_status status;

    switch (dc_value_type (context))
    {
    case DISIR_VALUE_TYPE_INTEGER:
    {
        status = dx_restriction_exclusive_value_check (context,
                                                       context->cx_keyval->kv_value.dv_integer,
                                                       0, NULL);
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        status = dx_restriction_exclusive_value_check (context, 0,
                                                       context->cx_keyval->kv_value.dv_float,
                                                       NULL);
        break;

    }
    case DISIR_VALUE_TYPE_ENUM:
    {
        status = dx_restriction_exclusive_value_check (context, 0, 0,
                                                       context->cx_keyval->kv_value.dv_string);
        break;
    }
    case DISIR_VALUE_TYPE_STRING:
    case DISIR_VALUE_TYPE_BOOLEAN:
        // Nothing to do for string and boolean
        status = DISIR_STATUS_OK;
        break;
    default:
    {
        log_warn ("Checking exclusive restrictions of unknown value type: %s.",
                  dc_value_type_string (context));
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    return status;
}

//! STATIC API
//!
//! Check the input context will exceed a finalized parents max restriction for it.
//!
//! \return DISIR_STATUS_INTERNAL_ERROR if context is not KEYVAL or SECTION.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if maximum entries for this context
//!         is violated in parent restriction.
//! \return DISIR_STATUS_OK if parent is not finalized.
//! \return DISIR_STATUS_OK on success.
//!
static enum disir_status
validate_finalized_parent_constructing_child_violating_max_restriction (struct disir_context *child)
{
    int max;
    int current_entries_count;
    struct disir_collection *collection;
    char *name;

    collection = NULL;
    max = 0;
    current_entries_count  = 0;

    // We are finalized - skip this check
    if (child->CONTEXT_STATE_FINALIZED == 1)
    {
        return DISIR_STATUS_OK;
    }
    // The parent is not finalized - skip this check
    if (child->cx_parent_context->CONTEXT_STATE_FINALIZED == 0)
    {
        return DISIR_STATUS_OK;
    }

    // Get name of current entry
    if (dc_context_type (child) == DISIR_CONTEXT_KEYVAL)
    {
        name = child->cx_keyval->kv_name.dv_string;
    }
    else if (dc_context_type (child) == DISIR_CONTEXT_SECTION)
    {
        name = child->cx_section->se_name.dv_string;
    }
    else
    {
        log_fatal ("Context unhandled (%s): %s", dc_context_type_string (child), __FILE__);
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    // Query all entries in parent with our name. Check if it is exhausted.
    dx_restriction_entries_value (child, DISIR_RESTRICTION_INC_ENTRY_MAX, NULL, &max);
    dc_find_elements (child->cx_parent_context, name, &collection);
    if (collection)
    {
        current_entries_count = dc_collection_size (collection);
        dc_collection_finished (&collection);
    }

    if (max != 0 && max <= current_entries_count)
    {
        // Dis-allow finalizing this entry.
        dx_log_context (child, "Maximum entries of %d exceeded", max);
        return DISIR_STATUS_RESTRICTION_VIOLATED;
    }

    return DISIR_STATUS_OK;
}

//! STATIC API
//!
//! \return DISIR_STATUS_ELEMENTS_INVALID if any of context' children are not valid.
//! \return DISIR_STATUS_OK when all children are valid.
//!
static enum disir_status
validate_children (struct disir_context *context)
{
    enum disir_status status;
    enum disir_status status_validate;
    enum disir_status invalid;
    struct disir_context *element;
    struct disir_collection *collection;

    invalid = DISIR_STATUS_OK;
    status_validate = DISIR_STATUS_OK;
    element = NULL;
    collection = NULL;

    // Invoke recursively on children
    status = dc_get_elements (context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        log_debug (2, "validating children failed to retrieve elements with status: %s",
                      disir_status_string (status));
        return status;
    }

    do
    {
        // Put back the element resource from last iteration
        if (element)
        {
            status = dc_putcontext (&element);
            if (status != DISIR_STATUS_OK)
                break;
        }

        // XXX: What if the last context was finalized? We should still validate, shant we?
        // Break out if a serious error occurred in last iteration
        if (status_validate != DISIR_STATUS_OK
            && status_validate != DISIR_STATUS_CONFLICTING_SEMVER
            && status_validate != DISIR_STATUS_ELEMENTS_INVALID
            && status_validate != DISIR_STATUS_INVALID_CONTEXT
            && status_validate != DISIR_STATUS_RESTRICTION_VIOLATED
            && status_validate != DISIR_STATUS_WRONG_VALUE_TYPE
            && status_validate != DISIR_STATUS_MOLD_MISSING
            && status_validate != DISIR_STATUS_DEFAULT_MISSING)
        {
            status = status_validate;
            // TODO: Verify that this scenario occurs and find a reasonable way to deal with it.
            log_fatal ("XXX: VALIDATE CHILDREN RETURNED NON-OK (NON-INVALID) status: %s",
                       disir_status_string (status));
            break;
        }

        status = dc_collection_next (collection, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        else if (status != DISIR_STATUS_OK)
        {
            break;
        }

        status_validate = dx_validate_context (element);
        // Only update invalid with either DISIR_STATUS_OK or the previous value of invalid.
        invalid = (status_validate != DISIR_STATUS_OK ? DISIR_STATUS_ELEMENTS_INVALID : invalid);
    } while (1);

    if (collection)
    {
        dc_collection_finished (&collection);
    }

    return (status == DISIR_STATUS_OK ? invalid : status);
}


//! STATIC API
//!
//! \return DISIR_STATUS_MOLD_MISSING if keyval doesnt have a mold equivalent.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if keyval is finalized
//!         and violating parent max restrictions.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if mold value type differs from keyval value type.
//! \return DISIR_STATUS_OK on valid keyval.
//!
static enum disir_status
validate_config_keyval (struct disir_context *keyval)
{
    enum disir_status status;
    struct disir_context *mold_equiv;

    mold_equiv = keyval->cx_keyval->kv_mold_equiv;

    // XXX: Should we check for empty name? This scenario is unlikely.

    // If no mold equivalent, this entry is invalid.
    if (mold_equiv == NULL)
    {
        dx_log_context (keyval, "KEYVAL missing mold equivalent entry for name '%s'.",
                                keyval->cx_keyval->kv_name.dv_string);
        return DISIR_STATUS_MOLD_MISSING;
    }

    status = validate_finalized_parent_constructing_child_violating_max_restriction (keyval);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Ensure that the correct type is assigned.
    // This might occur when constructing the config keyval. (unserializing)
    if (dc_value_type (mold_equiv) != dc_value_type (keyval))
    {
        dx_log_context (keyval, "Value type '%s' assigned. Expecting '%s'",
                                dc_value_type_string (mold_equiv), dc_value_type_string (keyval));
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    // Verify value
    status = validate_exclusive_restrictions (keyval);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return DISIR_STATUS_OK;
}

//! STATIC API
//!
//! \return DISIR_STATUS_MOLD_MISSING if keyval doesnt have a mold equivalent.
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if
//!         and violating parent max restrictions.
//!     Also returned when
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if mold value type differs from section value type.
//! \return DISIR_STATUS_OK on valid section.
//!
static enum disir_status
validate_config_section (struct disir_context *section)
{
    enum disir_status status;
    struct disir_context *mold_equiv;

    mold_equiv = section->cx_section->se_mold_equiv;

    // If no mold equivalent, this entry is invalid.
    if (mold_equiv == NULL)
    {
        dx_log_context (section, "SECTION missing mold equivalent entry for name '%s'.",
                                 section->cx_section->se_name.dv_string);
        return DISIR_STATUS_MOLD_MISSING;
    }

    status = validate_finalized_parent_constructing_child_violating_max_restriction (section);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Ensure that the correct type is assigned.
    // This might occur when constructing the config section. (unserializing)
    if (dc_value_type (mold_equiv) != dc_value_type (section))
    {
        dx_log_context (section, "Value type '%s' assigned. Expecting '%s'",
                                 dc_value_type_string (mold_equiv),
                                 dc_value_type_string (section));
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    status = validate_inclusive_restrictions (section);
    return status;
}

//! STATIC API
static enum disir_status
validate_restriction (struct disir_context *context)
{
    enum disir_status status;
    struct disir_restriction *res;
    struct disir_restriction **queue;

    queue = NULL;
    res = context->cx_restriction;

    // check if type is set
    if (dx_restriction_type_sanify (res->re_type) == DISIR_RESTRICTION_UNKNOWN)
    {
       dx_log_context (context, "Missing type component for restriction.");
       return DISIR_STATUS_INVALID_CONTEXT;
    }

    // Validate that the restriction context got a valid type set (belongs to a group)
    status = dx_restriction_get_queue (context, &queue);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Verify conflicting restrictions on the parent.
    switch (res->re_type)
    {
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
        // FALL-THROUGH
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    {
        // Verify that the introduced version does not conflict wither other entries.
        MQ_FOREACH (*queue,
        {
            // Check that we compare the same type of restriction - and not our selves.
            if (entry->re_type == res->re_type && entry->re_context != (void *)context)
            {
                // Check introduced does not conflict with any other entries
                if (dc_version_compare (&entry->re_introduced, &res->re_introduced) == 0)
                {
                    log_debug (6, "introduced conflict (entry %p vs context %p)",
                                  entry->re_context, context);
                    dx_context_error_set (context,
                                          "introduced version conflicts with" \
                                          " another %s restriction.",
                                          dc_restriction_enum_string (res->re_type));
                    status = DISIR_STATUS_CONFLICTING_SEMVER;
                    break;
                }
                // Deprecated cannot conflict
            }
        });

        break;
    }
    default:
        // No validation required
        break;
    }


    return status;
}

//! STATIC API
static enum disir_status
validate_name (struct disir_context *context)
{
    enum disir_status status;
    const char *name;
    const char *c;

    status = dc_get_name (context, &name, NULL);
    if (status != DISIR_STATUS_OK)
        return status;

    // Deny names that are not a-z or _
    c = name;
    while (*c)
    {
        if (*c >= 'a' && *c <= 'z')
        {
            c++;
            continue;
        }
        if (*c == '_')
        {
            c++;
            continue;
        }
        if (*c >= '0' && *c <= '9')
        {
            c++;
            continue;
        }

        status = DISIR_STATUS_RESTRICTION_VIOLATED;
        dx_log_context (context, "name contains invalid character '%c'.", *c);
        break;
    }

    return status;
}

//! STATIC API
//!
//! Check various conditions on the input context. Return codes indicate
//! the nature of its invalidity. Will also check validity of child contexts.
//!
//! \return DISIR_STATUS_ELEMENTS_INVALID if any of the contexts children are invalid.
//! \return DISIR_STATUS_MOLD_MISSING if a context whose top-level is CONFIG, is missing
//!         a mold equivalent entry.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the mold equivalent context type differs
//!         from the input context type.
//! \return DISIR_STATUS_OK on success.
//!
static enum disir_status
validate_context_validity (struct disir_context *context)
{
    enum disir_status status;
    enum disir_status invalid;

    // variable 'invalid' preserves the most important state.
    invalid = DISIR_STATUS_OK;
    // variable 'status' is used to propagate fatal operational errors.
    status = DISIR_STATUS_OK;

    switch (dc_context_type (context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        invalid = validate_inclusive_restrictions (context);
        if (invalid != DISIR_STATUS_OK && invalid != DISIR_STATUS_RESTRICTION_VIOLATED)
        {
            // Something is horribly wrong - break out.
            break;
        }
        __attribute__ ((fallthrough));
    }
    case DISIR_CONTEXT_MOLD:
    {
        // TODO: Clear error reports
        status = validate_children (context);
        // Update invalid with new state, if non were already present.
        invalid = (invalid == DISIR_STATUS_OK ? status : invalid);
        // Clear ELEMENTS_INVALID status if present - its not a fatal error to proagate
        status = (status == DISIR_STATUS_ELEMENTS_INVALID ? DISIR_STATUS_OK : status);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        invalid = validate_name (context);

        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            // Invoke config operation
            status = validate_config_section (context);
            invalid = (invalid != DISIR_STATUS_OK ? invalid : status);
        }
        else
        {
            struct disir_restriction *current_restriction;
            current_restriction = context->cx_section->se_restrictions_queue;
            while (invalid == DISIR_STATUS_OK && current_restriction)
            {
                invalid = dx_validate_context (current_restriction->re_context);
                if (invalid != DISIR_STATUS_OK)
                {
                    dx_context_transfer_logwarn (context, current_restriction->re_context);
                    break;
                }
                current_restriction = current_restriction->next;
            }
        }

        // 'invalid' contains the status best representing the state
        // this context can be in that is _not_ ok.
        if (invalid != DISIR_STATUS_INVALID_CONTEXT
            && invalid != DISIR_STATUS_RESTRICTION_VIOLATED
            && invalid != DISIR_STATUS_MOLD_MISSING
            && invalid != DISIR_STATUS_WRONG_VALUE_TYPE
            && invalid != DISIR_STATUS_CONFLICTING_SEMVER
            && invalid != DISIR_STATUS_OK
            && invalid != DISIR_STATUS_FATAL_CONTEXT)
        {
            // Break out - something horrible happend
            log_warn ("Something amis happend in section validation: %s",
                       disir_status_string (invalid));
            break;
        }

        status = validate_children (context);
        // Update invalid with new state, if non were already present.
        invalid = (invalid == DISIR_STATUS_OK ? status : invalid);
        // Clear ELEMENTS_INVALID status if present - its not a fatal error to proagate
        status = (status == DISIR_STATUS_ELEMENTS_INVALID ? DISIR_STATUS_OK : status);
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        invalid = validate_name (context);

        if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_CONFIG)
        {
            // Invoke config operation
            status = validate_config_keyval (context);
            invalid = (invalid != DISIR_STATUS_OK ? invalid : status);
        }
        else if (dc_context_type (context->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            // Invoke mold operation
            // Cannot add without known type.
            if (invalid == DISIR_STATUS_OK &&
                dx_value_type_sanify (context->cx_keyval->kv_value.dv_type) == DISIR_VALUE_TYPE_UNKNOWN)
            {
                dx_log_context (context, "Missing type component for keyval.");
                invalid = DISIR_STATUS_WRONG_VALUE_TYPE;
            }

            // Additional restrictions apply for keyvals added to a root mold
            // Only check if we are not already in erroneous state
            if (invalid == DISIR_STATUS_OK &&
                context->cx_keyval->kv_default_queue == NULL)
            {
                dx_log_context (context, "Missing default entry for keyval.");
                invalid = DISIR_STATUS_DEFAULT_MISSING;
            }

            struct disir_default *current_default;
            current_default = context->cx_keyval->kv_default_queue;
            while (invalid == DISIR_STATUS_OK && current_default != NULL)
            {
                invalid = dx_validate_context (current_default->de_context);
                if (invalid != DISIR_STATUS_OK)
                {
                    dx_context_transfer_logwarn (context, current_default->de_context);
                    break;
                }
                current_default = current_default->next;
            }

            struct disir_restriction *current_restriction;
            current_restriction = context->cx_keyval->kv_restrictions_queue;
            while (invalid == DISIR_STATUS_OK && current_restriction)
            {
                invalid = dx_validate_context (current_restriction->re_context);
                if (invalid != DISIR_STATUS_OK)
                {
                    dx_context_transfer_logwarn (context, current_restriction->re_context);
                    break;
                }
                current_restriction = current_restriction->next;
            }
        }
        break;
    }
    case DISIR_CONTEXT_DEFAULT:
    {
        // TODO: Add some validation to default context
        break;
    }
    case DISIR_CONTEXT_RESTRICTION:
    {
        invalid = validate_restriction (context);
        break;
    }
    default:
    {
        log_fatal ("Invoked validate operation on unhandled context %s.",
                   dc_context_type_string (context));
        status = DISIR_STATUS_INTERNAL_ERROR;
    }
    }

    // status must be a fatal status, else return the invalid status
    return (status != DISIR_STATUS_OK ? status : invalid);
}

//! INTERNAL API
enum disir_status
dx_validate_context (struct disir_context *context)
{
    enum disir_status status;

    TRACE_ENTER ("context %s", dc_context_type_string(context));

    if (context->CONTEXT_STATE_FATAL)
    {
        log_debug_context (1, context, "in fatal state - not valid");
        context->CONTEXT_STATE_INVALID = 1;
        status = DISIR_STATUS_FATAL_CONTEXT;
        if (context->CONTEXT_STATE_CONSTRUCTING)
        {
            status = DISIR_STATUS_INVALID_CONTEXT;
        }
        goto out;
    }

    status = DISIR_STATUS_OK;

    // Optimistically clear INVALID state
    // The below checks will return it to invalid state if checks fail.
    context->CONTEXT_STATE_INVALID = 0;

    // XXX: This validity check may return any number of error conditions.
    // This is used to determine if finalization of calling context may be done.
    status = validate_context_validity (context);

    // If our parent context is finalized, and we get any sort of error, we mark invalid state
    // and return the original status back - we do not allow this context to be finalized
    // (which means that INVALID_CONTEXT is a terrible status on error conditions - should remove that)
    if (context->cx_parent_context
        && context->cx_parent_context->CONTEXT_STATE_FINALIZED
        && status != DISIR_STATUS_OK)
    {
        // if status is DISIR_STATUS_INVALID_CONTEXT, we have made a mistake earlier.
        if (status == DISIR_STATUS_INVALID_CONTEXT)
        {
            dx_crash_and_burn ("VALIDITY CHECK PROGRAMMER ERROR: RETURNED INVALID_CONTEXT");
        }

        // Do nothing - we are returning status as-is.
    }
    // We are invalid - mark us as such.
    // ELEMENTS_INVALID does NOT put the calling context into invalid state.
    else if (status == DISIR_STATUS_WRONG_VALUE_TYPE
             || status == DISIR_STATUS_DEFAULT_MISSING
             || status == DISIR_STATUS_FATAL_CONTEXT
             || status == DISIR_STATUS_MOLD_MISSING
             || status == DISIR_STATUS_CONFLICTING_SEMVER
             || status == DISIR_STATUS_RESTRICTION_VIOLATED)
    {
        context->CONTEXT_STATE_INVALID = 1;
        // QUESTION: Why do we check parent_context?
        //  Changed to check our own context instead. Document any side-effects when encountered.
        if (context->CONTEXT_STATE_CONSTRUCTING)
        {
            status = DISIR_STATUS_INVALID_CONTEXT;
        }
        else
        {
            log_debug (4, "context is not constructing.. Returning trivial error: %s",
                       disir_status_string (status));
        }
    }
    // Even though we ourselves are not invalid, our children are.
    // Since we are in constructing mode, we will return INVALID_CONTEXT instead
    else if (status == DISIR_STATUS_ELEMENTS_INVALID
             && context->CONTEXT_STATE_CONSTRUCTING)
    {
        log_debug_context (4, context, "we are not invalid, but our children are." \
                           " Since we are constructing, returning INVALID_CONTEXT either way.");
        status = DISIR_STATUS_INVALID_CONTEXT;
    }
    else
    {
        log_debug (4, "validate context returned non-trivial error: %s",
                   disir_status_string (status));
    }

out:
    TRACE_EXIT ("%s", disir_status_string (status));
    return status;
}

//! INTERNAL API
enum disir_status
dx_invalid_elements (struct disir_context *context, struct disir_collection *collection)
{
    enum disir_status status;
    enum disir_status invalid;
    struct disir_collection *col;
    struct disir_context *element;

    invalid = (context->CONTEXT_STATE_INVALID == 1 ? DISIR_STATUS_INVALID_CONTEXT
                                                   : DISIR_STATUS_OK);

    // Handle the root specially
    if (context == context->cx_root_context)
    {
        if (collection && (invalid == DISIR_STATUS_INVALID_CONTEXT))
        {
            dc_collection_push_context (collection, context);
        }
    }

    status = dc_get_elements (context, &col);
    if (status != DISIR_STATUS_OK)
    {
        log_error ("Failed to retrieve elements from context: %s",
                    disir_status_string (status));
    }

    while (status == DISIR_STATUS_OK)
    {
        status = dc_collection_next (col, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            log_error ("Failed to retrieve element from collection: %s",
                       disir_status_string (status));
            break;
        }

        status = dc_context_valid (element);
        if (status == DISIR_STATUS_INVALID_CONTEXT)
        {
            if (collection)
            {
                dc_collection_push_context (collection, element);
            }
            invalid = status;
        }

        // Recurse the sections
        if (dc_context_type (element) == DISIR_CONTEXT_SECTION)
        {
            status = dx_invalid_elements (element, collection);
            if (status == DISIR_STATUS_INVALID_CONTEXT)
            {
                invalid = status;
            }
        }

        dc_putcontext (&element);
        status = DISIR_STATUS_OK;
    }

    dc_collection_finished (&col);

    return (status == DISIR_STATUS_OK ? invalid : status);
}

