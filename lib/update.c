#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <disir/disir.h>
#include <disir/context.h>

#include "config.h"
#include "default.h"
#include "section.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"
#include "update_private.h"

//! STATIC FUNCTION
static enum disir_status
retrieve_all_keyvals_recursively (struct disir_context *current, struct disir_collection *keyvals)
{
    enum disir_status status;
    struct disir_collection *coll = NULL;
    struct disir_context *context = NULL;
    struct disir_element_storage *elements = NULL;

    switch (current->cx_type)
    {
    case DISIR_CONTEXT_CONFIG:
    {
        elements = current->cx_config->cf_elements;
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        elements = current->cx_section->se_elements;
        break;
    }
    case DISIR_CONTEXT_KEYVAL:
    {
        return dc_collection_push_context (keyvals, current);
    }
    default:
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = dx_element_storage_get_all (elements, &coll);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    do
    {
        status = dc_collection_next (coll, &context);
        if (status != DISIR_STATUS_OK)
            break;

        status = retrieve_all_keyvals_recursively (context, keyvals);
        if (status != DISIR_STATUS_OK)
            break;

        dc_putcontext (&context);
    }
    while (1);

    if (coll)
        dc_collection_finished (&coll);
    if (context)
        dc_putcontext (&context);

    return status;
}

//! PUBLIC API
enum disir_status
disir_update_config (struct disir_config *config,
                     struct disir_version *target, struct disir_update **update)
{
    enum disir_status status;
    struct disir_config *config_at_target;
    int res;
    char buffer[512];
    struct disir_update *up;

    TRACE_ENTER ("config: %p, update: %p", config, update);

    if (config == NULL || update == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s) (config: %p, update: %p)",
                   config, update);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (target == NULL)
    {
        // TODO: Check mold context is still valid.
        target = &config->cf_mold->mo_version;
        log_debug (4, "using highest version from mold: %s\n",
                   dc_version_string (buffer, 512, target));
    }

    res = dc_version_compare (&config->cf_version, target);
    if (res > 0)
    {
        log_warn ("Config has higher version (%s) than target (%s)",
                  dc_version_string (buffer, 256, &config->cf_version),
                  dc_version_string (buffer + 256, 256, target));
        return DISIR_STATUS_CONFLICTING_SEMVER;
    }
    if (res == 0)
    {
        // Nothing to be done - the config is already up-to-date
        log_debug (4, "Config and Schema are of equal version (%s)- nothing to be done",
                   dc_version_string (buffer, 512, &config->cf_version));
        return DISIR_STATUS_NO_CAN_DO;
    }

    up = calloc (1, sizeof (struct disir_update));
    if (up == NULL)
    {
        log_error ("failed to allocate memory for update structure\n");
        return DISIR_STATUS_NO_MEMORY;
    }

    status = disir_generate_config_from_mold (config->cf_mold, target, &config_at_target);
    if (status != DISIR_STATUS_OK)
    {
        log_error ("failed to generate config from mold\n");
        goto error;;
    }

    up->up_config_target = config_at_target;

    up->up_collection = dc_collection_create();

    status = retrieve_all_keyvals_recursively (config->cf_context, up->up_collection);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_EXHAUSTED)
        goto error;

    up->up_config_old = config;
    dc_version_set (&up->up_target, target);

    *update = up;
    return disir_update_continue (up);
error:
    disir_update_finished (&up, NULL);
    return status;
}

enum disir_status
disir_update_continue (struct disir_update *update)
{
    enum disir_status status;
    struct disir_context *config_keyval = NULL;
    struct disir_keyval *keyval;
    struct disir_default *config_def;
    struct disir_default *target_def;
    int32_t size;
    int res;

    config_keyval = NULL;

    TRACE_ENTER ("");

    if (update == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (update->up_keyval)
    {
        return DISIR_STATUS_CONFLICT;
    }

    while (1)
    {
        update->up_context_keyval = NULL;

        if (config_keyval)
        {
            dc_putcontext (&config_keyval);
        }

        status = dc_collection_next (update->up_collection, &config_keyval);
        if (status != DISIR_STATUS_OK)
            break;

        update->up_context_keyval = config_keyval;
        keyval = config_keyval->cx_keyval;

        // Get keyval default of target version - if version is less or equal to current
        // Do nothing
        dx_default_get_active (keyval->kv_mold_equiv, &update->up_target, &target_def);
        res = dc_version_compare (&target_def->de_introduced,
                                  &update->up_config_old->cf_version);
        if (res <= 0)
        {
            // Do nothing - mold default value for target is older or equal to config version
            continue;
        }
        // We might get lucky and have the current config value equal the target default value
        res = dx_value_compare (&keyval->kv_value, &target_def->de_value);
        if (res == 0)
        {
            // Phew - Lucky,
            // TODO: Add update report entry
            continue;
        }

        // Two cases:
        //  * Update config if value of default at config version equals entry in config
        //  * Conflict if default at config version differ from entry in config
        dx_default_get_active (keyval->kv_mold_equiv, &update->up_config_old->cf_version,
                               &config_def);
        res = dx_value_compare (&keyval->kv_value, &config_def->de_value);
        if (res == 0)
        {
            // Config has not changed since its last update value.
            // We can safely update the stored value in config with new default
            dx_value_copy (&keyval->kv_value, &target_def->de_value);
            // TODO: Add update report entry
            update->up_updated++;
        }
        else
        {
            // Oh-oh - We have a conflict
            // Target default is higher version than config value. Config value differ
            // from config default. Manual resolution is in order.
            update->up_keyval = keyval;

            update->up_config_value = malloc (512); // XXX Handle allocation better
            dx_value_stringify (&keyval->kv_value, 512, update->up_config_value, &size);
            // XXX Handle buffer overflow / insufficient buffer

            update->up_mold_value = malloc (512); // XXX: Handle allocation better
            dx_value_stringify (&target_def->de_value, 512, update->up_mold_value, &size);
            // XXX Handle buffer overflow / insufficient buffer

            // Signal that manual resolution is required.
            return DISIR_STATUS_CONFLICT;
        }
    }
    if (status != DISIR_STATUS_EXHAUSTED)
    {
        log_error ("Collection failed with erroenous condition: %s", disir_status_string (status));
        return status;
    }

    // Update version number of config
    dc_version_set (&update->up_config_old->cf_version, &update->up_target);

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

enum disir_status
disir_update_conflict (struct disir_update *update, const char **name,
                       const char **keyval, const char **mold)
{
    TRACE_ENTER ("");

    if (update == NULL || name == NULL || keyval == NULL || mold == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // There is no conflict on the update
    if (update->up_keyval == NULL)
    {
        return DISIR_STATUS_NO_CAN_DO;
    }

    *name = update->up_keyval->kv_name.dv_string;
    *keyval = update->up_config_value;
    *mold = update->up_mold_value;

    TRACE_EXIT ("");

    return DISIR_STATUS_OK;
}

enum disir_status
disir_update_resolve (struct disir_update *update, const char *resolve)
{
    enum disir_status status;
    struct disir_context *context_keyval = NULL;

    TRACE_ENTER ("");
    if (update == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (update->up_keyval == NULL)
    {
        // No conflict to resolve
        return DISIR_STATUS_NO_CAN_DO;
    }

    status = dc_query_resolve_context (update->up_config_target->cf_context,
                                       update->up_keyval->kv_name.dv_string, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto out;

    status = dc_set_value (context_keyval, resolve, strlen (resolve));
    if (status != DISIR_STATUS_OK)
        goto out;

    dc_putcontext (&update->up_context_keyval);

    // TODO: Add update report entry
    update->up_updated++;

    // Remove conflict state
    update->up_keyval = NULL;
    update->up_context_keyval = NULL;
    free (update->up_config_value);
    update->up_config_value = NULL;
    free (update->up_mold_value);
    update->up_mold_value = NULL;

    TRACE_EXIT ("");
    // FALL-THROUGH
out:
    if (context_keyval)
        dc_putcontext (&context_keyval);

    return status;
}

enum disir_status
disir_update_finished (struct disir_update **update, struct disir_config **config)
{
    struct disir_update *up;

    TRACE_ENTER ("");

    up = *update;

    if (config)
    {
        *config = up->up_config_target;
    }
    else
    {
        disir_config_finished (&up->up_config_target);
    }

    dc_collection_finished (&up->up_collection);
    if (up->up_config_value)
    {
        free (up->up_config_value);
    }
    if (up->up_mold_value)
    {
        free (up->up_mold_value);
    }
    if (up->up_context_keyval)
    {
        dc_putcontext (&up->up_context_keyval);
    }

    free (up);
    *update = NULL;

    TRACE_EXIT ("");

    return DISIR_STATUS_OK;
}

//! INTERNAL API
enum disir_status
dx_update_config_with_changes (struct disir_config **config, int discard_violations)
{
    enum disir_status status;
    struct disir_update *update = NULL;
    struct disir_config *c;
    const char *name;
    const char *keyval;
    const char *mold;

    status = disir_update_config (*config, &(*config)->cf_mold->mo_version, &update);
    if (status != DISIR_STATUS_CONFLICT)
        goto error;

    do
    {
        if (status != DISIR_STATUS_CONFLICT)
            break;

        status = disir_update_conflict (update, &name, &keyval, &mold);
        if (status != DISIR_STATUS_OK)
            goto error;

        status = disir_update_resolve (update, keyval);
        if (status != DISIR_STATUS_OK &&
            status == DISIR_STATUS_RESTRICTION_VIOLATED && discard_violations)
        {
            status = disir_update_resolve (update, mold);
        }
        else if (status != DISIR_STATUS_OK)
        {
            goto error;
        }

        status = disir_update_continue (update);

    }
    while (1);
   
    c = *config;

    disir_update_finished (&update, config);

    return disir_config_finished (&c);
error:
    if (update)
        disir_update_finished (&update, NULL);

    return status;
}

