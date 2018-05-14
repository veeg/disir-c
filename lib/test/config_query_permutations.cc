
static enum disir_status
config_query_permutations (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_section = NULL;
    struct disir_context *context_second = NULL;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // ROOT keyval
    // "root"
    status = dc_add_keyval_string (context_mold, "root", "string_value", "k1value doc",
                                   NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    // FIRST section
    // "first"
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &context_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_section, "first", strlen ("first"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_documentation (context_section, "doc", strlen ("doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_restriction_entries_max (context_section, 4, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_min (context_section, 2, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    // STRING keyval under FIRST
    // "first.key_string"
    status = dc_add_keyval_string (context_section, "key_string", "string_value", "k1value doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 3, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);


    // SECOND section
    // "first.second"
    status = dc_begin (context_section, DISIR_CONTEXT_SECTION, &context_second);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_second, "second", strlen ("second"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_documentation (context_second, "doc", strlen ("doc"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_second, 3, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;


    // INTEGER keyval under SECOND
    // "first.second.key_integer"
    status = dc_add_keyval_integer (context_second, "key_integer", 5, "key_integer doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 4, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);


    // Finalize SECOND
    status = dc_finalize (&context_second);
    if (status != DISIR_STATUS_OK)
        goto error;

    // MAXIMAL section
    // "first.maximal"
    status = dc_begin (context_section, DISIR_CONTEXT_SECTION, &context_second);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (context_second, "maximal", strlen ("maximal"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_documentation (context_second, "doc", strlen ("doc"));
    if (status != DISIR_STATUS_OK)
        goto error;


    // INTEGER keyval under MAXIMAL
    // "first.maximal.key_integer"
    status = dc_add_keyval_integer (context_second, "key_integer", 5, "key_integer doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (context_keyval, 1, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);

    // STRING keyval under MAXIMAL
    // "first.maximal.key_string"
    status = dc_add_keyval_string (context_second, "key_string", "test", "key_string doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_min (context_keyval, 1, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);

    // BOOLEAN keyval under MAXIMAL
    // "first.maximal.key_boolean"
    status = dc_add_keyval_boolean (context_second, "key_boolean", 0, "key_boolean doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_min (context_keyval, 1, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);

    // FLOAT keyval under MAXIMAL
    // "first.maximal.key_float"
    status = dc_add_keyval_float (context_second, "key_float", 3.14, "key_float doc",
                                   NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_min (context_keyval, 1, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);

    // ENUM keyval under MAXIMAL
    // "first.maximal.key_enum"
    status = dc_add_keyval_enum (context_second, "key_enum", "one",  "key_enum doc",
                                 NULL, &context_keyval);
    if (status != DISIR_STATUS_OK)
        goto error;

    // Add ENUM restrictions to value
    status = dc_add_restriction_value_enum (context_keyval, "one", "The first value known to god",
                                            NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_enum (context_keyval, "two", "The second value known to god",
                                            NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_value_enum (context_keyval, "third", "The third value known to god",
                                            NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    dc_putcontext (&context_keyval);


    // Finalize MAXIMAL
    status = dc_finalize (&context_second);
    if (status != DISIR_STATUS_OK)
        goto error;



    // Finalize FIRST
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

