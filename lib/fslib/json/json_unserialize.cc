// JSON private
#include "json/json_unserialize.h"

// 3party
#include "fdstream.hpp"

// public
#include <disir/disir.h>
#include <disir/fslib/json.h>
#include <disir/fslib/util.h>


//! FSLIB API
enum disir_status
dio_json_unserialize_config (struct disir_instance *instance, FILE *input,
                             struct disir_mold *mold, struct disir_config **config)
{
    disir_log_user (instance, "TRACE ENTER dio_json_unserialize_config");
    try
    {
        boost::fdistream file(fileno(input));
        dio::ConfigReader reader (instance, mold);

        return reader.unserialize (config, file);
    }
    catch (std::exception& e)
    {
        disir_log_user (instance, "JSON: fatal exception in unserialize_config");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
dio_json_unserialize_mold (struct disir_instance *instance,
                           FILE *input, struct disir_mold **mold)
{
    disir_log_user (instance, "TRACE ENTER dio_json_unserialize_mold");

    try
    {
        boost::fdistream file(fileno(input));
        dio::MoldReader reader (instance);

        return reader.unserialize (file, mold);
    }
    catch (std::exception& e)
    {
        disir_log_user (instance, "JSON: fatal exception in unserialize_mold");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

enum disir_status
dio_json_unserialize_mold_override (struct disir_instance *instance,
                                    FILE *namespace_input , FILE *override_input,
                                    struct disir_mold **mold)
{
    disir_log_user (instance, "TRACE ENTER dio_json_unserialize_mold_override");
    enum disir_status status;

    try
    {
        dio::MoldReader reader (instance);
        boost::fdistream override_file(fileno(override_input));

        status = reader.set_mold_override (override_file);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        boost::fdistream namespace_file(fileno(namespace_input));

        return reader.unserialize (namespace_file, mold);
    }
    catch (std::exception& e)
    {
        disir_log_user (instance, "JSON: fatal exception in unserialize_mold_override");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

enum disir_status
dio_json_determine_mold_override (struct disir_instance *instance, FILE *input)
{
    // Safe-guard
    try
    {
        dio::MoldReader reader (instance);
        boost::fdistream override_file (fileno (input));

        return reader.is_override_mold_entry (override_file);
    }
    catch (std::exception& e)
    {
        disir_log_user (instance, "JSON: fatal exception in determine_mold_override");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
}

enum disir_status
dio_json_unserialize_mold_filepath (struct disir_instance *instance,
                                    const char *filepath, const char *oe_filepath,
                                    struct disir_mold **mold)
{
    enum disir_status status;
    struct stat statbuf;
    FILE *mold_file = NULL;
    FILE *oe_file = NULL;

    disir_log_user (instance, "TRACE ENTER dio_json_unserialize_mold");

    // Check if file exists
    status = fslib_stat_filepath (instance, filepath, &statbuf);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    mold_file = fopen (filepath, "r");
    if (mold_file == NULL)
    {
        disir_error_set (instance, "opening for reading %s: %s", filepath, strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    if (oe_filepath)
    {
        oe_file = fopen (oe_filepath, "r");
        if (oe_file == NULL)
        {
            disir_error_set (instance, "opening for reading %s: %s", filepath, strerror (errno));
            status = DISIR_STATUS_FS_ERROR;
            goto out;
        }

        status = dio_json_unserialize_mold_override (instance, mold_file, oe_file, mold);
    }
    else
    {
        // Regular mold
        status = dio_json_unserialize_mold (instance, mold_file, mold);
    }
    // FALL-THROUGH
out:
    if (mold_file)
        fclose (mold_file);
    if (oe_file)
        fclose (oe_file);

    return status;
}

