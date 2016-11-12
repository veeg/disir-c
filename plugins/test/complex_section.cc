
enum disir_status
complex_section (struct disir_mold **mold)
{
    enum disir_status status;
    struct disir_context *context_mold;
    struct disir_context *single_section;
    struct disir_context *single_section_nested;
    struct disir_context *array_table;
    //struct disir_context *nested_array_table;
    //struct disir_context *nested_array_table_nest;

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    status = dc_add_documentation (context_mold, "test_doc", strlen ("test_doc"));
    if (status != DISIR_STATUS_OK)
        goto error;

    // Single section
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &single_section);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (single_section, "single_section", strlen ("single_section"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_keyval_boolean (single_section, "key_boolean", 0, "doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;

    // single_section_nested
    status = dc_begin (single_section, DISIR_CONTEXT_SECTION, &single_section_nested);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (single_section_nested, "nested", strlen ("nested"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_keyval_integer (single_section_nested, "key_integer", 42, "doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_finalize (&single_section_nested);
    if (status != DISIR_STATUS_OK)
        goto error;
    // finalize single_section
    status = dc_finalize (&single_section);
    if (status != DISIR_STATUS_OK)
        goto error;

    // array_table
    status = dc_begin (context_mold, DISIR_CONTEXT_SECTION, &array_table);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_set_name (array_table , "array_table", strlen ("array_table"));
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_restriction_entries_max (array_table, 2, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_add_keyval_string (array_table, "key_string", "Manchester", "doc", NULL, NULL);
    if (status != DISIR_STATUS_OK)
        goto error;
    status = dc_finalize (&array_table);
    if (status != DISIR_STATUS_OK)
        goto error;


    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
        goto error;

    return DISIR_STATUS_OK;
error:
    if (context_mold)
    {
        dc_destroy (&context_mold);
    }
    return status;
}

