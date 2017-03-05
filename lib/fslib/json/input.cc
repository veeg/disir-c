#include <disir/disir.h>
#include <iostream>
#include "input.h"

using namespace dio;

enum disir_status
dio_json_config_read (struct disir_instance *disir, const char *id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum dplugin_status pstatus;

    try {
        ConfigReader reader (disir, mold);

        pstatus = reader.unmarshal (config, id);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            if (pstatus == DPLUGIN_IO_ERROR)
            {
                return DISIR_STATUS_INVALID_ARGUMENT;
            }
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    } catch (std::exception& e) {
        disir_error_set (disir, "Json io plugin got an unexpected exception: %s",
                                e.what ());
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return disir_config_valid (*config, NULL);
}

enum disir_status
dio_json_config_version (struct disir_instance *disir,
                         const char *id, struct semantic_version *semver)
{
    enum dplugin_status pstatus;

    try {
        ConfigReader reader (disir);

        pstatus = reader.read_config_version (semver, id);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    } catch (...) {
        disir_error_set (disir, "Json io plugin got an unexpected exception");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    return DISIR_STATUS_OK;
}

enum disir_status
dio_json_config_list (struct disir_instance *disir,
                      struct disir_collection **collection)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}

enum disir_status
dio_json_mold_read (struct disir_instance *disir,
                    const char *id, struct disir_mold **mold)
{
    enum dplugin_status status;


    try {
        MoldReader reader (disir);

        status = reader.unmarshal (id, mold);
        if (status != DPLUGIN_STATUS_OK)
        {
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    } catch (std::exception& e) {
        disir_error_set (disir, "Json io plugin got an unexpected exception");
        return DISIR_STATUS_INTERNAL_ERROR;
    }
    return DISIR_STATUS_OK;
}

enum disir_status
dio_json_mold_list (struct disir_instance *disir,
                    struct disir_collection **collection)
{
    return DISIR_STATUS_INTERNAL_ERROR;
}



