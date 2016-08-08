
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

    status = dc_add_keyval_string (context, "key_string", "string_value", "k1value doc",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_integer (context, "key_integer", 42, "k2value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_float (context, "key_float", 3.14, "k3value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_boolean (context, "key_boolean", 1, "key boolean doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    return status;
}

