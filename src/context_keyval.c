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

//! INTERNAL API
enum disir_status
dx_keyval_begin (struct disir_context *parent, struct disir_context **keyval)
{
    enum disir_status status;
    struct disir_context *context;

    if (keyval == NULL)
    {
        log_debug ("invoked with keyval NULL pointer.");
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
        log_debug_context (parent, "failed to allocate new keyval context.");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_keyval = dx_keyval_create (context);
    if (context->cx_keyval == NULL)
    {
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new keyval instance.");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (parent, "allocated new keyval instance: %p", context->cx_keyval);

    dx_context_attach (parent, context);
    *keyval = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_keyval_finalize (struct disir_context **keyval)
{
    enum disir_status status;
    struct disir_element_storage *storage;

    status = CONTEXT_DOUBLE_NULL_INVALID_TYPE_CHECK (keyval);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Get parent element storage
    switch (dc_context_type ((*keyval)->cx_parent_context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        storage = (*keyval)->cx_parent_context->cx_config->cf_elements;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        storage = (*keyval)->cx_parent_context->cx_section->se_elements;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        storage = (*keyval)->cx_parent_context->cx_mold->mo_elements;
        break;
    }
    default:
    {
        dx_crash_and_burn ("%s: %s not supported - Impossible", __FUNCTION__,
                           dc_context_type_string ((*keyval)->cx_parent_context));
    }
    }

    // Cannot add keyval without a name
    if ((*keyval)->cx_keyval->kv_name.dv_string == NULL)
    {
        dx_context_error_set (*keyval, "Missing name component for keyval.");
        return DISIR_STATUS_INVALID_CONTEXT;
    }

    // Cannot add without known type.
    if (dx_value_type_sanify((*keyval)->cx_keyval->kv_value.dv_type) == DISIR_VALUE_TYPE_UNKNOWN)
    {
        dx_context_error_set (*keyval, "Missing type component for keyval.");
        return DISIR_STATUS_INVALID_CONTEXT;
    }


    // Additional restrictions apply for keyvals added to a root mold
    if (dc_context_type ((*keyval)->cx_root_context) == DISIR_CONTEXT_MOLD)
    {
        // Cannot add without atleast one default entry.
        if ((*keyval)->cx_keyval->kv_default_queue == NULL)
        {
            dx_context_error_set (*keyval, "Missing default entry for keyval.");
            return DISIR_STATUS_INVALID_CONTEXT;
        }
    }

    // TODO: Verify that adding this entry does not conflict with the restrictions
    // inplace for parent
    // We should probably allow to insert invalid entries, but mark them
    // as an invalid state and return a special status code indicating such.

    status = dx_element_storage_add (storage, (*keyval)->cx_keyval->kv_name.dv_string, *keyval);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context(*keyval, "Unable to insert into element storage - Interesting...");
        return status;
    }

    *keyval = NULL;

    return DISIR_STATUS_OK;
}

static enum disir_status
add_keyval_generic (struct disir_context *parent, const char *name, const char *doc,
                    struct semantic_version *semver, enum disir_value_type type,
                    const char *string_input,
                    double float_input,
                    int64_t integer_input,
                    uint8_t boolean_input
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

    status = dc_set_name (keyval, name, strlen (name));
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    switch (type)
    {
    case DISIR_VALUE_TYPE_STRING:
    {
        status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_STRING);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }

        status = dc_add_default_string (keyval, string_input, strlen (string_input), semver);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_INTEGER:
    {
        status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_INTEGER);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }

        status = dc_add_default_integer (keyval, integer_input, semver);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    case DISIR_VALUE_TYPE_FLOAT:
    {
        status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_FLOAT);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }

        status = dc_add_default_float (keyval, float_input, semver);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
        break;
    }
    default:
        dx_crash_and_burn ("%s: called with invalid/unhandled type: %s",
                           __FUNCTION__, dx_value_type_string (type));
    }

    if (semver)
    {
        status = dc_add_introduced (keyval, *semver);
        if (status != DISIR_STATUS_OK)
        {
            // Already logged
            goto error;
        }
    }

    status = dc_add_documentation (keyval, doc, strlen (doc));
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    status = dx_keyval_finalize (&keyval);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    return DISIR_STATUS_OK;
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
dc_add_keyval_string (struct disir_context *parent, const char *name, const char *def,
                      const char *doc, struct semantic_version *semver)
{
    return add_keyval_generic (parent, name, doc, semver,
                               DISIR_VALUE_TYPE_STRING,
                               def,
                               0,
                               0,
                               0
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_boolean (struct disir_context *parent, const char *name, uint8_t def,
                       const char* doc, struct semantic_version *semver)
{
    dx_crash_and_burn ("%s invoked - not implemented yet");
    return DISIR_STATUS_INTERNAL_ERROR;
    return add_keyval_generic (parent, name, doc, semver,
                               DISIR_VALUE_TYPE_BOOLEAN,
                               NULL,
                               0,
                               0,
                               def
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_float (struct disir_context *parent, const char *name, double def,
                     const char *doc, struct semantic_version *semver)
{
    return add_keyval_generic (parent, name, doc, semver,
                               DISIR_VALUE_TYPE_FLOAT,
                               NULL,
                               def,
                               0,
                               0
                               );
}

//! PUBLIC API
enum disir_status
dc_add_keyval_integer (struct disir_context *parent, const char *name, int64_t def,
                     const char *doc, struct semantic_version *semver)
{
    return add_keyval_generic (parent, name, doc, semver,
                               DISIR_VALUE_TYPE_INTEGER,
                               NULL,
                               0,
                               def,
                               0
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
    keyval->kv_introduced.sv_major = 1;

    return keyval;
}

//! INTERNAL API
enum disir_status
dx_keyval_destroy (struct disir_keyval **keyval)
{
    struct disir_context *context;
    struct disir_documentation *doc;
    struct disir_default *def;

    if (keyval == NULL || *keyval == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Free allocated name
    if ((*keyval)->kv_name.dv_size != 0)
    {
        free ((*keyval)->kv_name.dv_string);
    }

    // Free allocated value, if string
    if ((*keyval)->kv_value.dv_type == DISIR_VALUE_TYPE_STRING &&
        (*keyval)->kv_value.dv_size != 0)
    {
        free ((*keyval)->kv_value.dv_string);
    }

    // Decref mold_equiv if set
    if ((*keyval)->kv_mold_equiv)
    {
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

    free (*keyval);
    *keyval = NULL;
    return DISIR_STATUS_OK;
}

