// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// public disir interface
#include <disir/disir.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "section.h"
#include "config.h"
#include "mold.h"
#include "section.h"
#include "log.h"
#include "mqueue.h"

//! INTERNAL API
enum disir_status
dx_section_begin (struct disir_context *parent, struct disir_context **section)
{
    enum disir_status status;
    struct disir_context *context;

    if (section == NULL)
    {
        log_debug ("invoked with section NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    status = CONTEXT_NULL_INVALID_TYPE_CHECK (parent);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }
    status = CONTEXT_TYPE_CHECK (parent, DISIR_CONTEXT_CONFIG,
                                         DISIR_CONTEXT_MOLD,
                                         DISIR_CONTEXT_SECTION);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    context = dx_context_create (DISIR_CONTEXT_SECTION);
    if (context == NULL)
    {
        log_debug_context (parent, "failed to allocate new section context.");
        return DISIR_STATUS_NO_MEMORY;
    }

    context->cx_section = dx_section_create (context);
    if (context->cx_section == NULL)
    {
        dx_context_destroy (&context);
        dx_log_context (parent, "cannot allocate new section instance.");
        return DISIR_STATUS_NO_MEMORY;
    }

    log_debug_context (parent, "allocated new section instance: %p", context->cx_section);

    dx_context_attach (parent, context);
    *section = context;
    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_section_finalize (struct disir_context *section)
{
    enum disir_status status;
    struct disir_element_storage *storage;

    status = CONTEXT_NULL_INVALID_TYPE_CHECK (section);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    // Get parent element storage
    switch (dc_context_type (section->cx_parent_context))
    {
    case DISIR_CONTEXT_CONFIG:
    {
        storage = section->cx_parent_context->cx_config->cf_elements;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        storage = section->cx_parent_context->cx_section->se_elements;
        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        storage = section->cx_parent_context->cx_mold->mo_elements;
        break;
    }
    default:
    {
        dx_crash_and_burn ("%s: %s not supported - Impossible", __FUNCTION__,
                           dc_context_type_string (section->cx_parent_context));
    }
    }

    // Cannot add section without a name
    if (section->cx_section->se_name.dv_string == NULL)
    {
        dx_context_error_set (section, "Missing name component for section.");
        return DISIR_STATUS_INVALID_CONTEXT;
    }

    status = dx_element_storage_add (storage, section->cx_section->se_name.dv_string, section);
    if (status != DISIR_STATUS_OK)
    {
        dx_log_context(section, "Unable to insert into element storage - Interesting...");
        return status;
    }

    return DISIR_STATUS_OK;
}

//! INTERNAL API
struct disir_section *
dx_section_create (struct disir_context *self)
{
    struct disir_section *section;

    section = calloc(1, sizeof (struct disir_section));
    if (section == NULL)
        return NULL;

    section->se_name.dv_type = DISIR_VALUE_TYPE_STRING;
    section->se_context = self;
    section->se_elements = dx_element_storage_create ();
    if (section->se_elements == NULL)
    {
        goto error;
    }

    return section;
error:
    if (section && section->se_elements)
    {
        dx_element_storage_destroy (&section->se_elements);
    }
    if (section)
    {
        free (section);
    }
    return NULL;
}

//! INTERNAL API
enum disir_status
dx_section_destroy (struct disir_section **section)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_documentation *doc;
    struct disir_collection *collection;

    if (section == NULL || *section == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // Free allocated name
    if ((*section)->se_name.dv_size != 0)
    {
        free ((*section)->se_name.dv_string);
    }

    // Decref mold_equiv if set
    if ((*section)->se_mold_equiv)
    {
        dx_context_decref (&(*section)->se_mold_equiv);
        (*section)->se_mold_equiv = NULL;
    }

    // Destroy (all) documentation entries on the section.
    while ((doc = MQ_POP((*section)->se_documentation_queue)))
    {
        context = doc->dd_context;
        dc_destroy (&context);
    }

    // Destroy all element_storage children
    status = dx_element_storage_get_all ((*section)->se_elements, &collection);
    if (status == DISIR_STATUS_OK)
    {
        while (dc_collection_next (collection, &context) != DISIR_STATUS_EXHAUSTED)
        {
            dx_context_decref (&context);
        }
        dc_collection_finished (&collection);
    }
    else
    {
        log_warn ("failed to get_all from internal element storage: %s",
                  disir_status_string (status));
    }

    dx_element_storage_destroy (&(*section)->se_elements);

    free (*section);
    *section = NULL;

    return DISIR_STATUS_OK;
}

