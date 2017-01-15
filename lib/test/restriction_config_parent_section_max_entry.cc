
enum disir_status
restriction_config_parent_section_max_entry (struct disir_mold **mold)
{
    enum disir_status status;
    struct semantic_version semver;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_section = NULL;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_section, "section", strlen ("section"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_documentation (context_section, "doc", strlen ("doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_entries_max (context_section, 4, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    semver.sv_major = 2;
    semver.sv_minor = 0;
    semver.sv_patch = 0;
    status = dc_add_restriction_entries_max (context_section, 2, &semver);
    if (status != DISIR_STATUS_OK)
        goto error;


    // Add a simple keyval to the section so that it is non-empty
    status = dc_add_keyval_string (context_section, "key_string", "string_value", "k1value doc",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_finalize (&context_section);
    if (status != DISIR_STATUS_OK)
        goto error;

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

