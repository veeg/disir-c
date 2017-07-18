
static enum disir_status
restriction_keyval_numeric_types (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_integer = NULL;
    struct disir_context *context_float = NULL;
    struct disir_context *context_complex = NULL;


    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // Integer
    status = dc_add_keyval_integer (context_mold, "viterbi_encoders", 2, "kint doc", NULL,
                                    &context_integer);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_value_numeric (context_integer, 1, NULL, NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_numeric (context_integer, 2, NULL, NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_numeric (context_integer, 8, "TERRA_HR/JPSS1_SMD",
                                               NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_putcontext (&context_integer);
    if (status != DISIR_STATUS_OK)
        goto error;

    // Float
    status = dc_add_keyval_float (context_mold, "float_phi", 3.14, "float_phi doc", NULL,
                                  &context_float);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_value_range (context_float, 2.66, 4.56, NULL, NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_putcontext (&context_float);
    if (status != DISIR_STATUS_OK)
        goto error;


    // float_complex
    struct disir_version version;
    version.sv_major = 1;
    version.sv_minor = 1;
    status = dc_add_keyval_float (context_mold, "float_complex", 5.66, "float_complex doc", NULL,
                                  &context_complex);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_value_range (context_complex, 5.55, 10.14,
                                             NULL, NULL, NULL);
                                             //&context_restriction);
    if (status != DISIR_STATUS_OK)
        goto error;
    // TODO: Uncomment when dc_add_deprecated is implemented
    //status = dc_add_deprecated (context_restriction, &version);
    //if (status != DISIR_STATUS_OK)
    //    goto error;

    status = dc_add_restriction_value_numeric (context_complex, 66.69, "uncertain", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_numeric (context_complex, 13.37, "l33t", &version, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_range (context_complex, 60.0, 100.0, NULL, &version, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_putcontext (&context_complex);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = DISIR_STATUS_OK;
error:
    if (context_integer)
    {
        dc_putcontext (&context_integer);
    }
    if (context_float)
    {
        dc_putcontext (&context_float);
    }
    if (context_complex)
    {
        dc_putcontext (&context_complex);
    }

    if (context_mold)
    {
        dc_destroy (&context_mold);
    }
    return status;
}

