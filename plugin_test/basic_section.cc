

enum disir_status
basic_section(struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_section;
    struct disir_context *context_mold;


    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "k1", "k1value", "k1value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "k2", "k2value", "k2value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "k3", "k3value", "k3value doc", NULL, NULL);
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
    return status;
}

