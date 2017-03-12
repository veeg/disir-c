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
    return DISIR_STATUS_INTERNAL_ERROR;
}

