
static enum disir_status
restriction_config_parent_keyval_max_entry (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_version version;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;


    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // keyval
    status = dc_add_keyval_float (context_mold, "keyval", 3.14, "complex doc", NULL,
                                  &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 2, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    version.sv_major = 2;
    version.sv_minor = 0;
    status = dc_add_restriction_entries_max (context_keyval, 4, &version);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_putcontext (&context_keyval);
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

