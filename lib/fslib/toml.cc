#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>

#include <disir/disir.h>
#include <disir/fslib/toml.h>

// The headonly implementation of TOML.
#include "tinytoml/toml.h"

//! PLUGIN API
enum disir_status
dio_toml_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    // TODO: Resolve entry_id to filepath.

    // TODO: Verify that filepath exists.

    // TODO: Locate mold, if not supplied

    // Open, read and parse file using TOML.
    // Iterate TOML object and create a config.
    return DISIR_STATUS_INTERNAL_ERROR;
}

//! PLUGIN API
enum disir_status
dio_toml_config_write (struct disir_instance *instance, struct disir_plugin *plugin,
                       const char *entry_id, struct disir_config *config)
{

    // TODO: Resolve entry_id to filepath.
    // TODO: Verify that filepath exists.

    return DISIR_STATUS_INTERNAL_ERROR;
}
