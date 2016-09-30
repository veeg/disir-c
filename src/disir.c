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

#define INTERNAL_MOLD_DOCSTRING "The Disir Schema for the internal libdisir configuration."
#define LOG_FILEPATH_DOCSTRING "The full filepath to the logfile libdisir will output to."
#define MOLD_DIRPATH_DOCSTRING "The full directory path where libdisir will locate " \
            "it the installed molds to match against installed configuration files."

//! INTERNAL STATIC
static void
load_plugin (struct disir_instance *disir, const char *plugin_filepath)
{
    enum disir_status status;
    void *handle;
    struct disir_plugin *plugin;
    enum disir_status (*dio_reg)(struct disir_instance *);

    // Attempt to load the filepath dynamically
    handle = dlopen (plugin_filepath, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL)
    {
        disir_error_set (disir, "Plugin located at filepath could not be loaded: %s: %s",
                         plugin_filepath, dlerror());
        return;
    }

    dlerror();
    dio_reg = dlsym (handle, "dio_register_plugin");
    if (dio_reg == NULL)
    {
        disir_error_set (disir,
                         "Plugin could not locate symbol 'dio_register_plugin' in SO '%s': %s",
                         plugin_filepath, dlerror());
        dlclose (handle);
        return;
    }

    status = dio_reg (disir);
    if (status != DISIR_STATUS_OK)
    {
        dlclose (handle);
        return;
    }

    // Allocate some storage handler and store this shit
    plugin = calloc (1, sizeof (struct disir_plugin));
    if (plugin == NULL)
    {
        // Ship is going down - abandon
        return;
    }

    plugin->pl_dl_handler = handle;
    plugin->pl_filepath = strdup (plugin_filepath);

    MQ_ENQUEUE (disir->dio_plugin_queue, plugin);

    return;
}

//! INTERNAL STATIC
void
load_plugins_from_config (struct disir_instance *disir, struct disir_config *config)
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
        disir_error_set (disir, "Failed to retrieve context from config object.");
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
            load_plugin (disir, value);
        }

        dc_putcontext (&element);
    }

    dc_putcontext (&context);
    dc_collection_finished (&collection);
}

static enum disir_status
recurse_elements_valid (struct disir_context *context,
                        struct disir_collection *collection)
{
    enum disir_status status;
    enum disir_status invalid;
    struct disir_collection *col;
    struct disir_context *element;

    invalid = (context->CONTEXT_STATE_INVALID == 1 ? DISIR_STATUS_INVALID_CONTEXT
                                                   : DISIR_STATUS_OK);

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
            status = recurse_elements_valid (element, collection);
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

enum disir_status
disir_libdisir_mold (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;

    context = NULL;

    status = dc_mold_begin (&context);
    if (status != DISIR_STATUS_OK)
        return status;

    status = dc_add_documentation (context,
                                   INTERNAL_MOLD_DOCSTRING,
                                   strlen (INTERNAL_MOLD_DOCSTRING));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "log_filepath", "/var/log/disir.log",
                                   "Log filepath for the internal disir log", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "mold_dirpath", "/etc/disir/molds",
                                   "Root directory to resolve mold lookups from.", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "plugin_filepath", "/usr/lib/disir/plugins/",
                                   "Load specified I/O plugin", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_mold_finalize (&context, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    if (context)
    {
        dc_destroy (&context);
    }

    *mold = NULL;
    return status;
}

//! PUBLIC API
enum disir_status
disir_libdisir_config_to_disk (struct disir_instance *disir, struct disir_config *config,
                               const char *filepath)
{
    return dio_ini_config_write (disir, filepath, config);
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
        log_debug (0, "invoked with disir NULL pointer.");
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
disir_instance_destroy (struct disir_instance **disir)
{
    struct disir_plugin *plugin;
    struct disir_input *input;
    struct disir_output *output;

    if (disir == NULL || *disir == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    // Free loaded plugins
    while (1)
    {
        plugin = MQ_POP ((*disir)->dio_plugin_queue);
        if (plugin == NULL)
            break;

        dlclose (plugin->pl_dl_handler);
        free (plugin->pl_filepath);
        free (plugin);
    }

    // Free loaded input
    while (1)
    {
        input = MQ_POP ((*disir)->dio_input_queue);
        if (input == NULL)
            break;

        free (input->di_type);
        free (input->di_description);
        free (input);
    }

    // Free loaded output
    while (1)
    {
        output = MQ_POP ((*disir)->dio_output_queue);
        if (output == NULL)
            break;

        free (output->do_type);
        free (output->do_description);
        free (output);
    }

    disir_config_finished(&(*disir)->libdisir_config);
    disir_mold_finished(&(*disir)->libdisir_mold);

    // Free any error message set on instance
    if ((*disir)->disir_error_message)
    {
        free ((*disir)->disir_error_message);
    }

    free (*disir);

    *disir = NULL;
    return DISIR_STATUS_OK;
}

static enum disir_status
generate_config_from_mold_recursive_step (struct disir_context *mold_parent,
                                          struct disir_context *config_parent,
                                          struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_collection *collection;
    struct disir_context *equiv;
    struct disir_context *context;
    struct disir_default *def;
    const char *name;
    int32_t size;
    int min_entries;
    int i;

    // Get each element from the element storage of mold_parent
    status = dc_get_elements (mold_parent, &collection);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    while (dc_collection_next (collection, &equiv) != DISIR_STATUS_EXHAUSTED)
    {

        status = dx_restriction_entries_value (equiv, DISIR_RESTRICTION_INC_ENTRY_MIN,
                                               semver, &min_entries);
        if (status != DISIR_STATUS_OK)
        {
            // XXX
            goto error;
        }
        if (min_entries == 0)
        {
            min_entries = 1;
        }

        // Generate min_entries entries of this element.
        for (i = 0; i < min_entries; i++)
        {
            status = dc_begin (config_parent, dc_context_type (equiv), &context);
            if (status != DISIR_STATUS_OK)
            {
                // Already logged
                goto error;
            }

            status = dc_get_name (equiv, &name, &size);
            if (status != DISIR_STATUS_OK)
            {
                log_debug (2, "failed to get name (%s). output size: %d\n",
                           disir_status_string (status), size);
                goto error;
            }

            status = dc_set_name (context, name, size);
            if (status != DISIR_STATUS_OK)
            {
                log_debug (2, "failed to add name: %s", disir_status_string (status));
                goto error;
            }

            if (dc_context_type (equiv) == DISIR_CONTEXT_KEYVAL)
            {
                // Get default entry matching semver
                //

                dx_default_get_active (equiv, semver, &def);

                status = dx_value_copy (&context->cx_keyval->kv_value, &def->de_value);
                if (status != DISIR_STATUS_OK)
                {
                    log_debug (2, "failed to copy value: %s", disir_status_string (status));
                    goto error;
                }
            }
            else if (dc_context_type (equiv) == DISIR_CONTEXT_SECTION)
            {
                // Send down parent and context -
                generate_config_from_mold_recursive_step (equiv, context, semver);
            }

            status = dc_finalize (&context);
            if (status != DISIR_STATUS_OK)
            {
                goto error;
            }
        }

        dc_putcontext (&equiv);
    }

    status = DISIR_STATUS_OK;
error:
    if (collection)
    {
        dc_collection_finished (&collection);
    }

    return status;

}

//! PUBLIC API
enum disir_status
disir_generate_config_from_mold (struct disir_mold *mold, struct semantic_version *semver,
                                   struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *config_context;
    char buffer[512];

    TRACE_ENTER ("mold: %p, semver: %p", mold, semver);

    if (mold == NULL)
    {
        log_debug (0, "invoked with mold NULL pointer");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    // TODO: Validate integrity of mold first?

    status = dc_config_begin (mold, &config_context);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        return status;
    }

    generate_config_from_mold_recursive_step (mold->mo_context, config_context, semver);

    status = dc_config_finalize (&config_context, config);
    if (status != DISIR_STATUS_OK)
    {
        // Already logged
        goto error;
    }

    if (semver)
    {
        dc_semantic_version_set (&(*config)->cf_version, semver);
    }
    else
    {
        dc_semantic_version_set (&(*config)->cf_version, &mold->mo_version);
    }
    log_debug (6, "sat config version to: %s",
               dc_semantic_version_string (buffer, 32, &(*config)->cf_version));

    TRACE_EXIT ("config: %p", *config);
    return DISIR_STATUS_OK;
error:
    if (config_context)
    {
        dc_destroy (&config_context);
    }

    return status;
}

//! PUBLIC API
enum disir_status
disir_config_valid (struct disir_config *config, struct disir_collection **collection)
{
    enum disir_status status;
    struct disir_collection *col;

    col = NULL;

    if (config == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    if (collection)
    {
        col = dc_collection_create ();
    }

    status = recurse_elements_valid (config->cf_context, col);

    // Iterate the config - retrieve all children and call dx_context_valid on it.
    // Retrieve element strorage - pass it to recursive function

    if (collection)
    {
        if (dc_collection_size (col))
        {
            *collection = col;
        }
        else
        {
            dc_collection_finished (&col);
        }
    }

    return status;
}

//! PUBLIC API
void
disir_log_user (struct disir_instance *disir, const char *message, ...)
{
    va_list args;

    if (disir == NULL || message == NULL)
        return;

    va_start (args, message);
    dx_log_disir (DISIR_LOG_LEVEL_USER, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, message, args);
    va_end (args);
}

//! PUBLIC API
void
disir_error_set (struct disir_instance *disir, const char *message, ...)
{
    va_list args;

    if (disir == NULL || message == NULL)
        return;

    va_start (args, message);
    dx_log_disir (DISIR_LOG_LEVEL_ERROR, 0, NULL, disir, 0, NULL, NULL, 0, NULL, message, args);
    va_end (args);
}

//! PUBLIC API
void
disir_error_clear (struct disir_instance *disir)
{
    if (disir->disir_error_message_size != 0)
    {
        disir->disir_error_message_size = 0;
        free (disir->disir_error_message);
        disir->disir_error_message = NULL;
    }
}

//! PUBLIC API
enum disir_status
disir_error_copy (struct disir_instance *disir,
                  char *buffer, int32_t buffer_size, int32_t *bytes_written)
{
    enum disir_status status;
    int32_t size;

    if (disir == NULL || buffer == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = DISIR_STATUS_OK;

    if (buffer_size < 4)
    {
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    size = disir->disir_error_message_size;
    if (bytes_written)
    {
        // Write the total size of the error message
        // If insufficient, return status will indicate and passed
        // buffer will be filled with partial error message
        *bytes_written = size;
    }
    if (buffer_size <= size)
    {
        size -= 4;
        status = DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }

    memcpy (buffer, disir->disir_error_message, size);
    if (status == DISIR_STATUS_INSUFFICIENT_RESOURCES)
    {
        sprintf (buffer + size, "...");
        size += 3;
    }
    buffer[size] = '\0';

    return status;
}

//! PUBLIC API
const char *
disir_error (struct disir_instance *disir)
{
    return disir->disir_error_message;
}

