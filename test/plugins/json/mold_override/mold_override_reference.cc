
static enum disir_status
retrieve_default_context (struct disir_context *context_keyval,
                          struct disir_version *semver_target,
                          struct disir_context **context_default)
{
    enum disir_status status;
    struct disir_collection *collection = NULL;
    struct disir_context *context = NULL;
    struct disir_version semver;

    status = dc_get_default_contexts (context_keyval, &collection);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    while (dc_collection_next (collection, &context) != DISIR_STATUS_EXHAUSTED)
    {
        status = dc_get_introduced (context, &semver);
        if (status != DISIR_STATUS_OK)
            goto out;

        if (dc_version_compare (&semver, semver_target) == 0)
        {
            *context_default = context;
            context = NULL;
            goto out;
        }
    }

    status = DISIR_STATUS_NOT_EXIST;
    // FALL-THROUGH
out:
    if (context)
        dc_putcontext (&context);
    if (collection)
        dc_collection_finished (&collection);

    return status;
}

static enum disir_status
override_basic_keyval (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_default = NULL;
    struct disir_version semver;

    context_mold = dc_mold_getcontext (*mold);

    status = dc_find_element (context_mold, "key_string", 0, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1,0};

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_string (context_default, "override_string",
                                    strlen ("override_string"));
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_default);

    status = dc_find_element (context_mold, "key_integer", 0, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_integer (context_default, 1);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_default);

    status = dc_find_element (context_mold, "key_float", 0, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_float (context_default, 6.14);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_default);

    status = dc_find_element (context_mold, "key_boolean", 0, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_boolean (context_default, 0);
    if (status != DISIR_STATUS_OK)
        goto error;
    // FALL-THROUGH
error:
    if (context_keyval)
        dc_putcontext (&context_keyval);
    if (context_default)
        dc_putcontext (&context_default);
    if (context_mold)
        dc_putcontext (&context_mold);

    return status;
}

static enum disir_status
override_json_test_mold (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_default = NULL;
    struct disir_version semver;

    context_mold = dc_mold_getcontext (*mold);

    status = dc_query_resolve_context (context_mold, "section_name.k1", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1, 7};

    status = dc_add_default_string (context_keyval, "override_string",
                                    strlen ("override_string"), &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1, 0};

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_string (context_default, "override_string_1_0_0",
                                  strlen ("override_string_1_0_0"));
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_default);
    dc_putcontext (&context_keyval);

    status = dc_query_resolve_context (context_mold, "section_name.integer", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1, 5};

    status = dc_add_default_integer (context_keyval, 1, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_query_resolve_context (context_mold, "section_name.float", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {2, 0};
    status = dc_add_default_float (context_keyval, 6.14, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_query_resolve_context (context_mold, "section_name.section2.k3", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {2, 0};
    status = dc_add_default_string (context_keyval, "override_string",
                                    strlen ("override_string"), &semver);
    if (status != DISIR_STATUS_OK)
        goto error;
    // FALL-THROUGH
error:
    if (context_keyval)
        dc_putcontext (&context_keyval);
    if (context_default)
        dc_putcontext (&context_default);
    if (context_mold)
        dc_putcontext (&context_mold);

    return status;
}

static enum disir_status
override_complex_section_equal_version (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_default = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_version semver;

    context_mold = dc_mold_getcontext (*mold);

    status = dc_query_resolve_context (context_mold, "single_section.key_boolean", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1,0};

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_boolean (context_default, 1);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_default);

    status = dc_query_resolve_context (context_mold, "single_section.nested.key_integer", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = retrieve_default_context (context_keyval, &semver, &context_default);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_value_integer (context_default, 1);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    dc_putcontext (&context_default);
    // FALL-THROUGH
error:
    if (context_keyval)
        dc_putcontext (&context_keyval);
    if (context_default)
        dc_putcontext (&context_default);
    if (context_mold)
        dc_putcontext (&context_mold);

    return status;
}

static enum disir_status
override_restriction_config_parent_keyval_min_entry (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_version semver;

    context_mold = dc_mold_getcontext (*mold);

    status = dc_query_resolve_context (context_mold, "keyval", &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {1,7};

    status = dc_add_default_integer (context_keyval, 170, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    semver = {2,0};

    status = dc_add_default_integer (context_keyval, 200, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);
    // FALL-THROUGH
error:
    if (context_keyval)
        dc_putcontext (&context_keyval);
    if (context_mold)
        dc_putcontext (&context_mold);

    return status;
}

