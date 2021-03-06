// JSON private
#include "json/json_serialize.h"

// 3party
#include "fdstream.hpp"

// public
#include <disir/disir.h>
#include <disir/fslib/json.h>


//! FSLIB API
enum disir_status
dio_json_serialize_config (struct disir_instance *instance,
                           struct disir_config *config, FILE* output)
{
    disir_log_user (instance, "TRACE ENTER dio_json_serialize_config");

    try
    {
        boost::fdostream file(fileno(output));
        dio::ConfigWriter writer (NULL);

         return writer.serialize (config, file);
    }
    catch (std::exception& e)
    {
        return DISIR_STATUS_INTERNAL_ERROR;
    }
}

// FSLIB API
enum disir_status
dio_json_serialize_mold (struct disir_instance *instance,
                         struct disir_mold *mold, FILE *output)
{
    disir_log_user (instance, "TRACE ENTER dio_json_serialize_mold");

    try
    {
        boost::fdostream file(fileno(output));
        dio::MoldWriter writer (instance);

        return writer.serialize (mold, file);
    }
    catch (std::exception& e)
    {
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    return DISIR_STATUS_OK;
}

