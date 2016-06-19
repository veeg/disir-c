
enum disir_status
basic_keyval(struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context;


    status = dc_mold_begin (&context);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "k1", "k1value", "k1value doc", NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "k2", "k2value", "k2value doc", NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context, "k3", "k3value", "k3value doc", NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    return status;
}

