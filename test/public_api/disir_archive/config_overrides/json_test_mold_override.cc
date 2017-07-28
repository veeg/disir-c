#include <disir/disir.h>

enum disir_status
json_test_mold_override (struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;

    status = disir_generate_config_from_mold (mold, NULL,  config);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return status;
}
