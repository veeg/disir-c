#include <stdlib.h>
#include <string.h>

#include <disir/disir.h>
#include <disir/context.h>

#include "config.h"
#include "default.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"
#include "update_private.h"

//! PUBLIC API
enum disir_status
disir_update_config (struct disir_config *config,
                     struct semantic_version *target, struct disir_update **update)
{
    enum disir_status status;
    int res;
    char buffer[512];
    struct disir_update *up;

    TRACE_ENTER ("config: %p, update: %p", config, update);

    if (config == NULL || update == NULL)
    {
        log_debug ("invoked with NULL pointer(s) (config: %p, update: %p)",
                   config, update);
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (target == NULL)
    {
        // TODO: Check mold context is still valid.
        target = &config->cf_context_mold->cx_mold->mo_version;
        log_debug ("using highest version from mold: %s\n",
                   dc_semantic_version_string (buffer, 512, target));
    }

    res = dc_semantic_version_compare (&config->cf_version, target);
    if (res > 0)
    {
        log_warn ("Config has higher version (%s) than target (%s)",
                  dc_semantic_version_string (buffer, 256, &config->cf_version),
                  dc_semantic_version_string (buffer + 256, 256, target));
        return DISIR_STATUS_CONFLICTING_SEMVER;
    }
    if (res == 0)
    {
        // Nothing to be done - the config is already up-to-date
        log_debug ("Config and Schema are of equal version (%s)- nothing to be done",
                   dc_semantic_version_string (buffer, 512, &config->cf_version));
        return DISIR_STATUS_NO_CAN_DO;
    }

    up = calloc (1, sizeof (struct disir_update));
    if (up == NULL)
    {
        log_error ("failed to allocate memory for update structure\n");
        return DISIR_STATUS_NO_MEMORY;
    }

    status = dx_element_storage_get_all (config->cf_elements, &up->up_collection);
    if (status != DISIR_STATUS_OK)
    {
        log_error ("Could not retrieve elements on config: %s\n", disir_status_string (status));
        free (up);
        return status;
    }

    up->up_config = config;
    dc_semantic_version_set (&up->up_target, target);

    *update = up;
    return disir_update_continue (up);

}

enum disir_status
disir_update_continue (struct disir_update *update)
{
    enum disir_status status;
    struct disir_context *config_keyval;
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
        status = dc_collection_next (update->up_collection, &config_keyval);
        if (status != DISIR_STATUS_OK)
            break;

        keyval = config_keyval->cx_keyval;

        // Get keyval default of target semver - if semver is less or equal to current
        // Do nothing
        dx_default_get_active (keyval->kv_mold_equiv, &update->up_target, &target_def);
        res = dc_semantic_version_compare (&target_def->de_introduced,
                                           &update->up_config->cf_version);
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

        dx_default_get_active (keyval->kv_mold_equiv, &update->up_config->cf_version,
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
            // Target default is higher semver than config value. Config value differ
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
    dc_semantic_version_set (&update->up_config->cf_version, &update->up_target);

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

    // set keyval value
    // TODO: Switch with dc_set_value call when implemented / supporting more types
    status = dc_set_value_string (update->up_keyval->kv_context, resolve, strlen (resolve));
    if (status != DISIR_STATUS_OK)
    {
        // No value set - did not validate?
        return status;
    }

    // TODO: Add update report entry
    update->up_updated++;

    // Remove conflict state
    update->up_keyval = NULL;
    free (update->up_config_value);
    update->up_config_value = NULL;
    free (update->up_mold_value);
    update->up_mold_value = NULL;

    TRACE_EXIT ("");
    return DISIR_STATUS_OK;
}

enum disir_status
disir_update_finished (struct disir_update **update)
{
    struct disir_update *up;

    up = *update;

    dc_collection_finished (&up->up_collection);
    if (up->up_config_value)
    {
        free (up->up_config_value);
    }
    if (up->up_mold_value)
    {
        free (up->up_mold_value);
    }

    free (up);
    *update = NULL;

    return DISIR_STATUS_OK;
}

