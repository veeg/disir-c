
static enum disir_status
multiple_defaults (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_version version;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_section = NULL;
    struct disir_context *context_section_nested = NULL;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_mold, "keyval_string", "1", "test keyval",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    version.sv_major = 1;
    version.sv_minor = 2;
    status = dc_add_default_string (context_keyval, "keyval_1_2_0",
                                    strlen ("keyval_1_2_0"), &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_add_keyval_integer (context_mold, "integer", (uint64_t)120, "integer doc",
                                    NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_default_integer (context_keyval, 1, &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_add_keyval_float (context_mold, "float", (double)3.14, "floatdoc",
                                  NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_default_float (context_keyval, (double)15.1, &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_value_range (context_keyval, (double)10.0, (double)20.0, NULL,
                                             &version, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_boolean (context_section, "bool", 1, "boolvalue doc", NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_default_boolean (context_keyval, 0, &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_begin (context_section, DISIR_CONTEXT_SECTION, &context_section_nested);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_name (context_section_nested, "section_nested", strlen ("section_nested"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section_nested, "keyval_nested_section", "k3value",
                                   "k1value doc", NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_default_string (context_keyval, "keyval_1_2_0",
                                    strlen ("keyval_1_2_0"), &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_finalize (&context_section_nested);
     if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_finalize (&context_section);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    if (context_section)
    {
        dc_destroy (&context_section);
    }
    if (context_mold)
    {
        dc_destroy (&context_mold);
    }
    if (context_section)
    {
        dc_destroy (&context_section);
    }
    if (context_section_nested)
    {
        dc_destroy (&context_section_nested);
    }
    return status;
}
