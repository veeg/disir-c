#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <disir/disir.h>
#include <disir/io.h>

#include "log.h"
#include "mold.h"
#include "config.h"

enum disir_status
dio_ini_config_write (const char *id, struct disir_config *config)
{
    enum disir_status status;
    dc_t *config_context;
    dc_t *context;
    dcc_t *collection;
    int32_t size;
    const char *name;
    const char *value;
    char buffer[512];
    char filename[512];
    char tmp_filepath[512];
    struct semantic_version semver;
    FILE *file;
    int res;
    int errsv;

    // Validate input ID
    // TODO: Generalize input ID validation to disk?
    int id_size;
    char *base;
    char *current;
    char *dir;

    config_context = NULL;
    context = NULL;
    name = NULL;
    value = NULL;
    file = NULL;
    collection = NULL;
    status = DISIR_STATUS_OK;

    id_size = strlen (id);
    if (id[id_size] == '/')
    {
        fprintf (stderr, "Cannot write config to directory path.. (trailing slash)\n");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }
    memset (filename, 0, 512);
    memcpy (filename, id, id_size);
    dir = base = current = filename;

    while (current < filename + id_size)
    {
        if (*current == '/')
            base = current;
        current++;
    }

    // set dir to NULL if base is equal, which means base makes up the
    // entire filename (excluding leading slash)
    if (dir == base)
        dir = NULL;
    else
        *base = '\0';

    base++;

    // Create temporary filename
    // XXX: Handle large ish filenames?
    snprintf (tmp_filepath, 512, "%s/.%s.tmp",
            (dir == NULL ? "" : dir),
            base);
    file = fopen (tmp_filepath, "w");
    if (file == NULL)
    {
        errsv = errno;
        fprintf (stderr, "Failed to open file: %s\n", strerror (errsv));
        status = DISIR_STATUS_INTERNAL_ERROR; // XXX
        goto error;
    }

    config_context = dc_config_getcontext (config);
    if (config_context == NULL)
    {
        fprintf (stderr, "Failed to get config context\n");
        status = DISIR_STATUS_INTERNAL_ERROR;; //XXX
        goto error;
    }

    status = dc_get_elements (config_context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Failed to get elements of config context\n");
        goto error;
    }

    status = dc_get_version (config_context, &semver);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Failed to get version of config context\n");
        goto error;
    }

    fprintf (file, "DISIR_CONFIG_VERSION = %s\n",
             dc_semantic_version_string (buffer, 512, &semver));

    while (dc_collection_next (collection, &context) == DISIR_STATUS_OK)
    {
        status = dc_get_name (context, &name, &size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stderr, "(fetching name failed");
            goto error;
        }
        else
        {
            fprintf (file, "%s = ", name);
        }

        // TODO: Switch on type
        status = dc_get_value_string (context, &value, &size);
        if (status != DISIR_STATUS_OK)
        {
            fprintf (stderr, "(fetching value failed)\n");
            goto error;
        }
        else
        {
            fprintf (file, "%s\n", value);
        }
    }

    dc_collection_finished (&collection);
    dc_putcontext (&config_context);

    res = fclose (file);
    if (res != 0)
    {
        fprintf (stderr, "Unable to close temporary output file: %d\n", res);
        status = DISIR_STATUS_INTERNAL_ERROR; // XXX
        goto error;
    }

    res = rename (tmp_filepath, id);
    if (res != 0)
    {
        fprintf (stderr, "Unable to rename '%s' to '%s' with res: %d\n", filename, id, res);
        status = DISIR_STATUS_INTERNAL_ERROR; // XXX
        goto error;
    }

    return DISIR_STATUS_OK;
error:
    if (collection)
    {
        dc_collection_finished (&collection);
    }

    if (file)
    {
        fclose (file);
        remove (filename);
    }

    if (config_context)
    {
        dc_putcontext (&config_context);
    }

    return status;
}

enum disir_status
dio_ini_mold_write (const char *id, struct disir_mold *mold)
{
    fprintf (stderr, "Output does not support outputting molds in INI format.\n");
    return DISIR_STATUS_NO_CAN_DO;
}

enum disir_status
dio_ini_config_read (const char *id, struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    FILE *file;
    int errsv;

    char *line;
    size_t line_buffer_size;
    ssize_t line_size;
    char *key;
    char *value;
    char *current;
    char *seperator;
    struct semantic_version semver;
    dc_t *config_context;
    dc_t *keyval;

    keyval = config_context = NULL;
    line = key = value = current = seperator = NULL;
    file = NULL;

    file = fopen (id, "r");
    if (file == NULL)
    {
        errsv = errno;
        fprintf (stderr, "Failed to open input '%s' with error: %s\n", id, strerror (errsv));
        return DISIR_STATUS_INVALID_ARGUMENT; // XXX
    }

    status = dc_config_begin (mold, &config_context);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "Failed to begin config: %s", disir_status_string (status));
        goto error;
    }


    line = NULL;
    while (1)
    {
        line_size = getline (&line, &line_buffer_size, file);
        if (line_size == -1)
        {
            // EOF?
            break;
        }

        // Ignore comment character
        if (line[0] == '#')
            continue;

        // Overwrite the newline character, if it exists (it should...)
        if (line[line_size - 1] == '\n')
        {
            line[line_size - 1] = '\0';
        }

        // Locate the equals keyval separator
        key = value = line;
        while (*value && *value != '=') { value++; };
        seperator = value;
        if (*value == '\0')
        {
            fprintf (stderr, "Input error - Line does not contain a separating '=': %s\n", line);
            status = DISIR_STATUS_INTERNAL_ERROR; // XXX Parse error;
            goto error;
        }

        // Move to non-space element
        value++;
        while (*value && *value == ' ') { value++; };
        if (value >= line + line_size)
        {
            fprintf (stderr,
                     "Input error - Line does not contain a value element after '=': %s\n", line);
            status = DISIR_STATUS_INTERNAL_ERROR; // XXX: Parse error:
            goto error;
        }

        // Set first whitespace element to null, if it exists (trailing whitespace)
        current = value;
        while (*current && *current != ' ') { current++; };
        *current = '\0';

        // Move keyval to non-space element
        while (*key && *key == ' ') { key++; };
        if (key >= seperator)
        {
            fprintf (stderr, "Input error - key does not contain any name before '=': %s\n", line);
            status = DISIR_STATUS_INTERNAL_ERROR; // XXX: Parse error
            goto error;
        }

        // Set first whitespace element to null, if it exists (trailing whitespace)
        current = key;
        while (*current && *current != ' ') { current++; };
        *current = '\0';

        if (strcmp (key, "DISIR_CONFIG_VERSION") == 0)
        {
            // Set config version to config
            status = dc_semantic_version_convert (value, &semver);
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "Invalid version number in config version: %s", value);
                status = DISIR_STATUS_INTERNAL_ERROR; // XXX: Parse error
                goto error;
            }
            status =  dc_set_version (config_context, &semver);
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "failed to set config version to %s: %s\n",
                         value, disir_status_string (status));
                goto error;
            }
        }
        else
        {
            // Construct keyval
            status = dc_begin (config_context, DISIR_CONTEXT_KEYVAL, &keyval);
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "failed to initialize context on keyval: %s\n",
                         disir_status_string (status));
                goto error;
            }
            status = dc_set_name (keyval, key, strlen (key));
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "failed to set name %s: %s\n", key, disir_status_string (status));
                goto error;
            }

            // TODO: Switch on type or add generic dc_set_value ()
            status = dc_set_value_string (keyval, value, strlen (value));
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "failed to set value string %s: %s\n",
                         value, disir_status_string (status));
                goto error;
            }

            status = dc_finalize (&keyval);
            if (status != DISIR_STATUS_OK)
            {
                fprintf (stderr, "failed to finalize keyval %s: %s\n",
                         key, disir_status_string (status));
                goto error;
            }
        }
    }

    status = dc_config_finalize (&config_context, config);
    if (status != DISIR_STATUS_OK)
    {
        fprintf (stderr, "failed to finalize config: %s\n", disir_status_string (status));
        goto error;
    }

    if (line)
    {
        free (line);
    }

    fclose (file);


    return DISIR_STATUS_OK;
error:
    if (file)
    {
        fclose (file);
    }
    if (config_context)
    {
        dc_destroy (&config_context);
    }
    if (keyval)
    {
        dc_destroy (&keyval);
    }
    if (line)
    {
        free (line);
    }
    return status;
}

enum disir_status
dio_ini_mold_read (const char *id, struct disir_mold **mold)
{
    fprintf (stderr, "Input does not support reading molds in INI format.\n");
    return DISIR_STATUS_NO_CAN_DO;
}

enum disir_status
dio_register_ini (struct disir *disir)
{
    enum disir_status status;

    log_info ("Registering INI I/O type");


    status = disir_register_output (disir, "INI", "Output to filesystem in INI format.",
                                    dio_ini_config_write, dio_ini_mold_write);
    if (status != DISIR_STATUS_OK)
    {
        log_error ("registering output INI failed with errenous condition: %s",
                   disir_status_string (status));
    }
    status = disir_register_input (disir, "INI", "Input from filesystem in INI format.",
                                    dio_ini_config_read, dio_ini_mold_read);
    if (status != DISIR_STATUS_OK)
    {
        log_error ("registering input INI failed with errenous condition: %s",
                   disir_status_string (status));
    }
    return status;
}


