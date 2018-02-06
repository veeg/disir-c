// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "keyval.h"
#include "config.h"
#include "mold.h"
#include "section.h"
#include "log.h"
#include "mqueue.h"
#include "restriction.h"

//! INTERNAL API
enum disir_status
dx_keyval_begin (struct disir_context *parent, struct disir_context **keyval)
{
    enum disir_status status;
    struct disir_context *context;

    if (keyval == NULL)
    {
        log_debug (0, "invoked with keyval NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_MOLD,
                                 DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // TODO: Capability / sanity check?
    // XXX: if parents grantparent is CONTEXT_CONFIG, check parent's mold reference
    // XXX: Should all these context have a direct mold pointer? To its equivilant mold
    //  entry?

    context = dx_context_create (DISIR_CONTEXT_KEYVAL);
    if (context == NULL)
    {
        log_debug_context (1, parent, "failed to allocate new keyval context.");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_keyval = dx_keyval_create (context);
    if (context->cx_keyval == NULL)
    {
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new keyval instance.");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (8, parent, "allocated new keyval instance: %p", context->cx_keyval);

    dx_context_attach (parent, context);
    *keyval = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_keyval_finalize (struct disir_context *keyval)
{
    enum disir_status status;
    enum disir_status invalid;
    struct disir_element_storage *storage;

    invalid = (keyval->CONTEXT_STATE_INVALID ? DISIR_STATUS_INVALID_CONTEXT
                                             : DISIR_STATUS_OK);

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (keyval);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Get parent element storage
    switch (dc_context_type (keyval->cx_parent_context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        storage = keyval->cx_parent_context->cx_config->cf_elements;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        storage = keyval->cx_parent_context->cx_section->se_elements;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        storage = keyval->cx_parent_context->cx_mold->mo_elements;
        break;
    }
    default:
    {
        dx_crash_and_burn ("%s: %s not supported - Impossible", __func__,
                           dc_context_type_string (keyval->cx_parent_context));
    }
    }

    // Cannot add keyval without a name
    // Cannot even add it to to storage (NO NAME!
    if (keyval->cx_keyval->kv_name.dv_string == NULL)
    {
        dx_log_context (keyval, "Missing name component for keyval.");
        // XXX: Cannot add to parent - Or can it? Add it to element storage with NULL name should still
        // add it to sequential list - just not the hashmap. Hmm..
        return DISIR_STATUS_CONTEXT_IN_WRONG_STATE;
    }

    invalid = dx_validate_context (keyval);

    if (invalid == DISIR_STATUS_OK ||
        invalid == DISIR_STATUS_INVALID_CONTEXT)
    {
        status = dx_element_storage_add (storage, keyval->cx_keyval->kv_name.dv_string, keyval);
        if (status != DISIR_STATUS_OK)
        {
            dx_log_context(keyval, "Unable to insert into element storage - Interesting...");
        }
        else
        {
            keyval->CONTEXT_STATE_IN_PARENT = 1;
        }
    }

    return (status == DISIR_STATUS_OK ? invalid : status);
}

static enum disir_status
add_keyval_generic (struct disir_context *parent, const char *name, const char *doc,
                    struct disir_version *version, enum disir_value_type type,
                    const char *string_input,
                    double float_input,
                    int64_t integer_input,
                    uint8_t boolean_input,
                    struct disir_context **output
                    )
{
    enum disir_status status;
    struct disir_context *keyval;

    keyval = NULL;

    status = dc_begin (parent, DISIR_CONTEXT_KEYVAL, &keyval);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    // Let DISIR_STATUS_NOT_FOUND through - mold equiv not set. context invalid.
    status = dc_set_name (keyval, name, strlen (name));
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
    {
        // Already logged
        goto error;
    }

    status = dc_set_value_type (keyval, type);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    switch (type)
    {
    case DISIR_VALUE_TYPE_ENUM:
        // QUESTION: Should adding a default string entry who does not match existing
        //  DISIR_RESTRICTION_EXC_VALUE_ENUM mark this status as invalid?

        // FALL-THROUGH
    case DISIR_VALUE_TYPE_STRING:
    {
        status = dc_add_default_string (keyval, string_input, strlen (string_input), version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        status = dc_add_default_integer (keyval, integer_input, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        status = dc_add_default_float (keyval, float_input, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_BOOLEAN:
    {
        status = dc_add_default_boolean (keyval, boolean_input, version);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_UNKNOWN:
        status = DISIR_STATUS_INTERNAL_ERROR;
    }

    status = dc_add_documentation (keyval, doc, strlen (doc));
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    if (output)
    {
        log_debug_context (9, keyval, "user provided output storage: incref context");
        dx_context_incref (keyval);
        *output = keyval;
    }

    status = dc_finalize (&keyval);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_INVALID_CONTEXT)
    {
        // Already logged
        if (output)
        {
            dx_context_decref (&keyval);
            *output = NULL;
        }
        goto error;
    }
    if (keyval)
    {
        // We were invalid, and we got another reference
        dc_putcontext (&keyval);
    }

    return status;
error:
    if (keyval)
    {
        dx_context_transfer_logwarn (parent, keyval);
        dc_destroy (&keyval);
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_add_keyval_enum (struct disir_context *parent, const char *name, const char *def,
                    const char *doc, struct disir_version *version,
                    struct disir_context **output)
{
    // TODO: How do we refine this function signature to take varadic number of arguments,
    // which we only want to add as restrictions to the enum? Hmm
    return add_keyval_generic (parent, name, doc, version,
                               DISIR_VALUE_TYPE_ENUM,
                               def,
                               0,
                               0,
                               0,
                               output
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_string (struct disir_context *parent, const char *name, const char *def,
                      const char *doc, struct disir_version *version,
                      struct disir_context **output)
{
    return add_keyval_generic (parent, name, doc, version,
                               DISIR_VALUE_TYPE_STRING,
                               def,
                               0,
                               0,
                               0,
                               output
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_boolean (struct disir_context *parent, const char *name, uint8_t def,
                       const char* doc, struct disir_version *version,
                       struct disir_context **output)
{
    return add_keyval_generic (parent, name, doc, version,
                               DISIR_VALUE_TYPE_BOOLEAN,
                               NULL,
                               0,
                               0,
                               def,
                               output
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_float (struct disir_context *parent, const char *name, double def,
                     const char *doc, struct disir_version *version,
                     struct disir_context **output)
{
    return add_keyval_generic (parent, name, doc, version,
                               DISIR_VALUE_TYPE_FLOAT,
                               NULL,
                               def,
                               0,
                               0,
                               output
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_integer (struct disir_context *parent, const char *name, int64_t def,
                       const char *doc, struct disir_version *version,
                       struct disir_context **output)
{
    return add_keyval_generic (parent, name, doc, version,
                               DISIR_VALUE_TYPE_INTEGER,
                               NULL,
                               0,
                               def,
                               0,
                               output
                               );
}

//! INTERNAL API
struct disir_keyval *
dx_keyval_create (struct disir_context *parent)
{
    struct disir_keyval *keyval;

    keyval = calloc(1, sizeof (struct disir_keyval));
    if (keyval == NULL)
        return NULL;

    keyval->kv_context = parent;
    keyval->kv_name.dv_type = DISIR_VALUE_TYPE_STRING;

    return keyval;
}

//! INTERNAL API
enum disir_status
dx_keyval_destroy (struct disir_keyval **keyval)
{
    struct disir_context *context;
    struct disir_documentation *doc;
    struct disir_default *def;
    struct disir_restriction *restriction;

    if (keyval == NULL || *keyval == NULL)
    {
        log_debug (0, "invoked with NULL keyval argument. (%p)", keyval);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Free allocated name
    if ((*keyval)->kv_name.dv_size != 0)
    {
        free ((*keyval)->kv_name.dv_string);
    }

    // Free allocated value, if string or enum
    if (((*keyval)->kv_value.dv_type == DISIR_VALUE_TYPE_STRING
        || (*keyval)->kv_value.dv_type == DISIR_VALUE_TYPE_ENUM)
           && (*keyval)->kv_value.dv_size != 0)
    {
        free ((*keyval)->kv_value.dv_string);
    }

    // Decref mold_equiv if set
    if ((*keyval)->kv_mold_equiv)
    {
        log_debug (5, "decrefing mold equivalent entry.");
        dx_context_decref (&(*keyval)->kv_mold_equiv);
        (*keyval)->kv_mold_equiv = NULL;
    }

    // Destroy (all) documentation entries on the keyval.
    while ((doc = MQ_POP((*keyval)->kv_documentation_queue)))
    {
        context = doc->dd_context;
        dc_destroy (&context);
    }

    // Destroy all default entries on the keyval.
    while ((def = MQ_POP((*keyval)->kv_default_queue)))
    {
        context = def->de_context;
        dc_destroy (&context);
    }

    // Destroy all restrictions
    while ((restriction = MQ_POP ((*keyval)->kv_restrictions_queue)))
    {
        context = restriction->re_context;
        dc_destroy (&context);
    }

    free (*keyval);
    *keyval = NULL;
    return DISIR_STATUS_OK;
}

//! PUBLIC API
enum disir_status
dc_keyval_set_default (struct disir_context *keyval, struct disir_version *version)
{
    enum disir_status status;
    struct disir_default *def = NULL;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (keyval);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (keyval, DISIR_CONTEXT_KEYVAL);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    if (dc_context_type (keyval->cx_root_context) != DISIR_CONTEXT_CONFIG)
    {
        log_debug(2, "cannot set value of KEYVAL to its default if its root is not CONFIG");
        return DISIR_STATUS_WRONG_CONTEXT;
    }

    if (keyval->cx_keyval->kv_mold_equiv == NULL)
    {
        log_warn("attempted to set value to default on a keyval who is missing a mold");
        return DISIR_STATUS_MOLD_MISSING;
    }

    dx_default_get_active (keyval->cx_keyval->kv_mold_equiv, version, &def);

    if (def == NULL)
    {
        log_warn("unable to get the default value from keyval");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    status = dx_value_copy (&keyval->cx_keyval->kv_value, &def->de_value);
    if (status != DISIR_STATUS_OK)
    {
        log_debug (2, "failed to copy value: %s", disir_status_string (status));
    }

    return status;
}
