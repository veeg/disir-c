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
static void
load_plugin (struct disir_instance *instance, const char *plugin_filepath, const char *plugin_name)
{
    enum disir_status status;
    void *handle;
    struct disir_plugin_internal *plugin;
    struct disir_plugin_internal *last_plugin;
    enum disir_status (*dio_reg)(struct disir_instance *, const char *);

    last_plugin = MQ_TAIL (instance->dio_plugin_queue);

    // Attempt to load the filepath dynamically
    handle = dlopen (plugin_filepath, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL)
    {
        disir_error_set (instance, "Plugin located at filepath could not be loaded: %s: %s",
                         plugin_filepath, dlerror());
        return;
    }

    dlerror();
    dio_reg = dlsym (handle, "dio_register_plugin");
    if (dio_reg == NULL)
    {
        disir_error_set (instance,
                         "Plugin could not locate symbol 'dio_register_plugin' in SO '%s': %s",
                         plugin_filepath, dlerror());
        dlclose (handle);
        return;
    }

    status = dio_reg (instance, plugin_name);
    if (status != DISIR_STATUS_OK)
    {
        dlclose (handle);
        return;
    }

    // Iterate backwards in the queue list. All entries between tail and last_plugin
    // are new entries added by dio_reg.
    // XXX: This probably doesnt work too well if multiple plugins are registered from the same
    // SO, since the dl_handle does not have multiple open references. Maybe we need to
    // do a dl_open X number of times to fool the reference count.
    // Oh well, problem for another day.
    plugin = MQ_TAIL (instance->dio_plugin_queue);
    while (plugin != last_plugin && plugin != NULL)
    {
        plugin->pi_dl_handler = handle;
        plugin->pi_filepath = strdup (plugin_filepath);
        plugin->pi_name = strdup (plugin_name);

        plugin = plugin->prev;
        // Guard against first insertion
        if (last_plugin == NULL)
            break;
    }

    return;
}

//! INTERNAL STATIC
void
load_plugins_from_config (struct disir_instance *instance, struct disir_config *config)
{
    enum disir_status status;
    struct disir_collection *collection;
    struct disir_context *element;
    struct disir_context *context;
    const char *name;
    const char *value;

    context = dc_config_getcontext (config);
    if (context == NULL)
    {
        disir_error_set (instance, "Failed to retrieve context from config object.");
        return;
    }

    status = dc_get_elements (context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        dc_putcontext (&context);
        return;
    }

    while (dc_collection_next (collection, &element) == DISIR_STATUS_OK)
    {
        status = dc_get_name (element, &name, NULL);
        if (status != DISIR_STATUS_OK)
        {
            log_warn ("libdisir config: Failed to get_name in element from collection: %s",
                      disir_status_string (status));
            continue;
        }

        status = dc_get_value_string (element, &value, NULL);
        if (status != DISIR_STATUS_OK)
        {
            log_warn ("libdisir config: Failed to get value string from elements: %s",
                      disir_status_string (status));
            continue;
        }

        if (strcmp (name, "plugin_filepath") == 0)
        {
            // TODO: Retrieve plugin name from config
            load_plugin (instance, value, "tmpname");
        }

        dc_putcontext (&element);
    }

    dc_putcontext (&context);
    dc_collection_finished (&collection);
}

//! PUBLIC API
enum disir_status
disir_libdisir_config_to_disk (struct disir_instance *instance, struct disir_config *config,
                               const char *filepath)
{
    return dio_ini_config_write (instance, filepath, config);
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
        status = dio_ini_config_read (dis, config_filepath, libmold, &libconf);
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
        if (plugin->pi_name)
            free (plugin->pi_name);
        if (plugin->pi_plugin.dp_name)
            free (plugin->pi_plugin.dp_name);
        if (plugin->pi_plugin.dp_type)
            free (plugin->pi_plugin.dp_type);
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

