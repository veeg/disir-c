// external public includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

// public disir interface
#include <disir/disir.h>
#include <disir/util.h>
#include <disir/context.h>

// private
#include "context_private.h"
#include "disir_private.h"
#include "config.h"
#include "mold.h"
#include "default.h"
#include "keyval.h"
#include "log.h"
#include "mqueue.h"
#include "restriction.h"

//! INTERNAL STATIC
static enum disir_status
load_plugin (struct disir_instance *instance, const char *plugin_filepath, const char *io_id,
             const char *group_id, const char *config_base_id)
{
    enum disir_status status;
    void *handle;
    struct disir_plugin plugin;
    struct disir_plugin_internal *internal;
    plugin_register dio_reg;

    // Attempt to load the filepath dynamically
    handle = dlopen (plugin_filepath, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL)
    {
        disir_error_set (instance, "Plugin located at filepath could not be loaded: %s: %s",
                         plugin_filepath, dlerror());
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    dlerror();
    dio_reg = dlsym (handle, "dio_register_plugin");
    if (dio_reg == NULL)
    {
        disir_error_set (instance,
                         "Plugin could not locate symbol 'dio_register_plugin' in SO '%s': %s",
                         plugin_filepath, dlerror());
        status = DISIR_STATUS_INVALID_ARGUMENT;
        goto error;
    }

    // Populate the plugin object
    memset (&plugin, 0, sizeof (struct disir_plugin));

    plugin.dp_config_base_id = strndup (config_base_id, 512);

    status = dio_reg (instance, &plugin);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    status = disir_plugin_register (instance, &plugin, io_id, group_id);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    internal = MQ_TAIL (instance->dio_plugin_queue);
    if (internal == NULL)
    {
        status = DISIR_STATUS_INTERNAL_ERROR;
        goto error;
    }
    internal->pi_dl_handler = handle;
    internal->pi_filepath = strndup (plugin_filepath, 512);
    internal->pi_io_id = strndup (io_id, 512);
    internal->pi_group_id = strndup (group_id, 512);

    return DISIR_STATUS_OK;
error:
    if (handle)
    {
        dlclose (handle);
    }

    return status;
}

//! INTERNAL STATIC
void
load_plugins_from_config (struct disir_instance *instance, struct disir_config *config)
{
    enum disir_status status;
    struct disir_collection *collection;
    struct disir_context *plugin;
    struct disir_context *context;
    const char *plugin_filepath;
    const char *group_id;
    const char *io_id;
    const char *config_base_id;

    context = dc_config_getcontext (config);
    if (context == NULL)
    {
        disir_error_set (instance, "Failed to retrieve context from config object.");
        return;
    }

    status = dc_find_elements (context, "plugin", &collection);
    if (status != DISIR_STATUS_OK)
    {
        dc_putcontext (&context);
        return;
    }

    while (dc_collection_next (collection, &plugin) == DISIR_STATUS_OK)
    {
        plugin_filepath = NULL;
        group_id = NULL;
        io_id = NULL;
        config_base_id = NULL;

        dc_config_get_keyval_string (plugin, &plugin_filepath, "plugin_filepath");
        dc_config_get_keyval_string (plugin, &group_id, "group_id");
        dc_config_get_keyval_string (plugin, &io_id, "io_id");
        dc_config_get_keyval_string (plugin, &config_base_id, "config_base_id");

        if (plugin_filepath && group_id && io_id && config_base_id)
        {
            load_plugin (instance, plugin_filepath, io_id, group_id, config_base_id);
        }
        else
        {
            // TODO: Improve this logging and/or handling
            log_error ("missing required keyval values.");
        }

        dc_putcontext (&plugin);
    }

    dc_putcontext (&context);
    dc_collection_finished (&collection);
}

// PUBLIC API
enum disir_status
disir_instance_create (const char *config_filepath, struct disir_config *config,
                       struct disir_instance **instance)
{
    enum disir_status status;
    struct disir_instance *dis;

    struct disir_config *libconf;
    struct disir_mold *libmold;

    status = DISIR_STATUS_OK;

    TRACE_ENTER ("config_filepath: %s, config: %p, instance: %p",
                 config_filepath, config, instance);

    if (instance == NULL)
    {
        log_debug (0, "invoked with instance NULL pointer.");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    dis = calloc (1, sizeof (struct disir_instance));
    if (dis == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    // We do not need to generate a mold entry - simply steal the one provided by the user.
    if (config == NULL)
    {
        status = disir_libdisir_mold (&libmold);
        if (status != DISIR_STATUS_OK)
        {
            log_debug (0, "failed do generate mold: %s", disir_status_string (status));
            goto error;
        }
    }

    if (config)
    {
        // Use user-provided config
        libconf = config;
        libmold = config->cf_mold;
    }
    else if (config_filepath)
    {
        // Read from disk
        status = disir_libdisir_config_from_disk (dis, config_filepath, libmold, &libconf);
    }
    else
    {
        // Load default config
        status = disir_generate_config_from_mold (libmold, NULL, &libconf);
    }

    if (status != DISIR_STATUS_OK)
    {
        log_error ("Failed to generate internal configuration: %s", disir_status_string (status));
        goto error;
    }

    // TODO: Validate libconf
    // XXX: Validate version? Upgrade?

    load_plugins_from_config (dis, libconf);

    dis->libdisir_mold = libmold;
    dis->libdisir_config = libconf;

    *instance= dis;
    TRACE_EXIT ("*instance: %p", *instance);
    return DISIR_STATUS_OK;
error:
    if (dis)
    {
        free (dis);
    }
    if (libmold)
    {
        disir_mold_finished (&libmold);
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_instance_destroy (struct disir_instance **instance)
{
    struct disir_plugin_internal *plugin;

    if (instance == NULL || *instance == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    // Free loaded plugins
    while (1)
    {
        plugin = MQ_POP ((*instance)->dio_plugin_queue);
        if (plugin == NULL)
            break;

        // Call cleanup method specified by plugin.
        if (plugin->pi_plugin.dp_plugin_finished)
        {
            plugin->pi_plugin.dp_plugin_finished (*instance, plugin->pi_plugin.dp_storage);
        }

        dlclose (plugin->pi_dl_handler);

        if (plugin->pi_filepath)
            free (plugin->pi_filepath);
        if (plugin->pi_io_id)
            free (plugin->pi_io_id);
        if (plugin->pi_plugin.dp_name)
            free (plugin->pi_plugin.dp_name);
        if (plugin->pi_plugin.dp_config_entry_type)
            free (plugin->pi_plugin.dp_config_entry_type);
        if (plugin->pi_plugin.dp_description)
            free (plugin->pi_plugin.dp_description);

        free (plugin);
    }

    disir_config_finished(&(*instance)->libdisir_config);
    disir_mold_finished(&(*instance)->libdisir_mold);

    // Free any error message set on instance
    if ((*instance)->disir_error_message)
    {
        free ((*instance)->disir_error_message);
    }

    free (*instance);

    *instance = NULL;
    return DISIR_STATUS_OK;
}

