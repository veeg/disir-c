

static enum disir_status
json_test_mold (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_section = NULL;
    struct disir_context *context_section_nested = NULL;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_mold, "test1", "1", "test keyval", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_mold, "test2", "1", "test keyval", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_integer (context_mold, "integer", (uint64_t)123, "integer doc",
                                    NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_float (context_mold, "float", (double)12.123, "floatdoc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_boolean (context_mold, "boolean", (uint8_t)8, "doc", NULL, NULL);
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

    status = dc_add_keyval_boolean (context_section, "bool", 1, "boolvalue doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_float (context_section, "float", 1.12, "floatvalue doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_integer (context_section, "integer", 123123, "intvalue doc",
                                    NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "k2", "k2value", "k2value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section, "k3", "k3value", "k3value doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_begin (context_section, DISIR_CONTEXT_SECTION, &context_section_nested);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_set_name (context_section_nested, "section2", strlen ("section2"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_keyval_string (context_section_nested, "k3", "k3value", "k1value doc",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

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
