// JSON private
#include "json/json_unserialize.h"

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

