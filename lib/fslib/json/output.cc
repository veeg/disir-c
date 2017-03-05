#include <map>
#include <disir/disir.h>
#include <string.h>
#include <log.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "output.h"

enum disir_status
dio_json_config_write (struct disir_instance *disir,
                       const char *id,
                       struct disir_config *config);

enum disir_status
dio_json_mold_write (struct disir_instance *disir,
                     const char *id,
                     struct disir_mold *mold);

int
dio_write_json (struct disir_instance *disir,
                const char *path,
                const std::string& json);


// Used by libdisir to output a config as json
enum disir_status
dio_json_config_write (struct disir_instance *disir, const char *id,
                       struct disir_config *config)
{
    int err;
    std::string json;

    try {
        dio::ConfigWriter writer (disir);

        auto pstatus = writer.marshal (config, json);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    } catch (...) {
        disir_error_set (disir, "recieved unhandled exception from json io plugin: ");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    err = dio_write_json (disir, id, json);
    if (err < 0) {
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

// Translates a disir_mold into json
enum disir_status
dio_json_mold_write (struct disir_instance *disir,
                     const char *id, struct disir_mold *mold)
{
    enum dplugin_status pstatus;
    std::string json_string;

    try {
        dio::MoldWriter writer (disir);

        pstatus = writer.marshal (mold, json_string);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    } catch (std::exception& e) {
        disir_error_set (disir, "recieved unhandled exception from json io plugin: ");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    auto err = dio_write_json (disir, id, json_string);
    if (err < 0)
    {
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

int
dio_write_json (struct disir_instance *disir, const char *path, const std::string& json)
{
    FILE *stream;
    size_t res;

    // w flag becase we don't care if
    //  another config file exists
    stream = fopen (path, "w");
    if (stream == NULL) {
        disir_error_set (disir, "Could not create config file (%s). Errno returned %s" ,
                                 path, strerror (errno));
        return -1;
    }

    res = fwrite (json.c_str(), sizeof (char), json.length(), stream);
    if (res < 0) {
        disir_error_set (disir, "could not write entire config to disk. Returned error: %s",
                                 strerror (errno));
        fclose (stream);
        return -1;
    }

    fclose (stream);

    return DISIR_STATUS_OK;
}
