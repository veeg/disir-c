// JSON private
#include "json/input.h"

// 3party
#include "fdstream.hpp"

// public
#include <disir/disir.h>
#include <disir/fslib/json.h>


//! FSLIB API
enum disir_status
dio_json_unserialize_config (struct disir_instance *instance, FILE *input,
                             struct disir_mold *mold, struct disir_config **config)
{
    disir_log_user (instance, "TRACE ENTER dio_json_unserialize_config");
    try
    {
        enum dplugin_status pstatus;
        boost::fdistream file(fileno(input));
        dio::ConfigReader reader (instance, mold);

        pstatus = reader.unmarshal (config, file);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            disir_log_user (instance, "JSON: unserialize_config failed");
            return DISIR_STATUS_INTERNAL_ERROR;
        }
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
        enum dplugin_status pstatus;
        boost::fdistream file(fileno(input));
        dio::MoldReader reader (instance);

        pstatus = reader.unmarshal (file, mold);
        if (pstatus != DPLUGIN_STATUS_OK)
        {
            disir_log_user (instance, "JSON: unserialize_mold failed");
            return DISIR_STATUS_INTERNAL_ERROR;
        }
    }
    catch (std::exception& e)
    {
        disir_log_user (instance, "JSON: fatal exception in unserialize_mold");
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

