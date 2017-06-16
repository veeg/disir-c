
static enum disir_status
restriction_entries (struct disir_mold **mold)
{
    enum disir_status status;
    struct semantic_version semver;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_section = NULL;


    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // default keyval
    status = dc_add_keyval_integer (context_mold, "keyval_default", 2, "kint doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    // default section
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_section, "section_default", strlen ("section_default"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // Add keyval to section so its non-empty
    status = dc_add_keyval_float (context_section, "nonempty", 3.14, "nonempty do docc", NULL,
                                  NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_finalize (&context_section);
    if (status != DISIR_STATUS_OK)
        goto error;


    // keyval_complex
    status = dc_add_keyval_float (context_mold, "keyval_complex", 3.14, "complex doc", NULL,
                                  &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_min (context_keyval, 2, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 2, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    semver.sv_major = 2;
    semver.sv_minor = 0;
    semver.sv_patch = 0;
    status = dc_add_restriction_entries_min (context_keyval, 3, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 4, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;

    dc_putcontext (&context_keyval);

    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = DISIR_STATUS_OK;
    // FALL-THROUGH
error:

    if (context_mold)
    {
        dc_destroy (&context_mold);
    }
    return status;
}

