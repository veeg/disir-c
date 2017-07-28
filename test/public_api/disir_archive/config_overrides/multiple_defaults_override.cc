#include <disir/disir.h>

enum disir_status
multiple_defaults_override (struct disir_config **config)
{
    enum disir_status status;
    struct disir_context *context_config;
    struct disir_context *context_keyval;

    context_config = dc_config_getcontext (*config);

    status = dc_query_resolve_context (context_config, "keyval_string", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_string (context_keyval, "keyval_userparam", strlen ("keyval_userparam"));
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_query_resolve_context (context_config, "integer", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_integer (context_keyval, 13);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_query_resolve_context (context_config, "section_name.bool", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_boolean (context_keyval, 0);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_config);

    return status;
error:
    if (context_keyval)
        dc_putcontext (&context_keyval);
    if (context_config)
        dc_putcontext (&context_config);
    return status;
}

enum disir_status
multiple_defaults_override_1_0 (struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_version version;

    version = {1,0};
    status = disir_generate_config_from_mold (mold, &version, config);
    if (status != DISIR_STATUS_OK)
        goto error;

    return multiple_defaults_override (config);
error:
    return status;
}

enum disir_status
multiple_defaults_1_0_restriction (struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_version version;
    struct disir_context *context_config;
    struct disir_context *context_keyval;

    version = {1,0};
    status = disir_generate_config_from_mold (mold, &version, config);
    if (status != DISIR_STATUS_OK)
        goto error;

    context_config = dc_config_getcontext (*config);

    status = dc_query_resolve_context (context_config, "float", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_float (context_keyval, 1.2);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_config);

    return multiple_defaults_override (config);
error:
    return status;
}

enum disir_status
multiple_defaults_override_1_2 (struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    struct disir_version version;

    version = {1,2};
    status = disir_generate_config_from_mold (mold, &version, config);
    if (status != DISIR_STATUS_OK)
        goto error;

    return multiple_defaults_override (config);
error:
    return status;
}
