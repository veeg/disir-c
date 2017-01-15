
enum disir_status
basic_version_difference (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_section;
    struct disir_context *context_mold;
    struct semantic_version semver;

    semver.sv_major = 2;
    semver.sv_minor = 0;
    semver.sv_patch = 0;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // key_deprecated_2_0_0
    status = dc_add_keyval_string (context_mold, "key_deprecated_2_0_0", "string_value", "k1value doc",
                                   NULL, &context);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_deprecated (context, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context);

    // key_introduced_2_0_0
    status = dc_add_keyval_integer (context_mold, "key_introduced_2_0_0", 42, "k2value doc",
                                    &semver, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    // Start a section that is deprecated in 3.0.0
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_section, "section_introduced_2_0_0",
                         strlen ("section_introduced_2_0_0"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_introduced (context_section, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;
    semver.sv_major = 3;
    status = dc_add_deprecated (context_section, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    // add key introduced 2.0.0
    status = dc_add_keyval_float (context_section, "key_introduced_2_0_0", 3.77, "k doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    // add key deprecated 2.5.0
    status = dc_add_keyval_integer (context_section, "key_deprecated_2_5_0", 3, "k3value doc",
                                    NULL, &context);
    if (status != DISIR_STATUS_OK)
        goto error;
    semver.sv_major = 2;
    semver.sv_minor = 5;
    status = dc_add_deprecated (context, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_putcontext (&context);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_finalize (&context_section);
    if (status != DISIR_STATUS_OK)
        goto error;

    // add key boolean introduced 2.6.0
    semver.sv_minor = 6;
    status = dc_add_keyval_boolean (context_mold, "key_introduced_2_6_0", 1, "bool doc", &semver, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    return status;
}

