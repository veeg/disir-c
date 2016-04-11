
#include <disir/disir.h>
#include <disir/collection.h>

void
test_context_get_elements (void **state)
{
    enum disir_status status;
    dc_t *invalid;
    dc_t *schema_context;
    dcc_t *collection;
    struct disir_schema *schema;

    LOG_TEST_START

    // Setup schema
    status = dc_schema_begin (&schema_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (schema_context, "keyval1",
                                   "keyval1_value", "keyval1_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (schema_context, "keyval2", "keyval2_value",
                                   "keyval2_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (schema_context, "keyval3", "keyval3_value",
                                   "keyval3_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (schema_context, "keyval4", "keyval4_value",
                                   "keyval4_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_schema_finalize (&schema_context, &schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    schema_context = dc_schema_getcontext (schema);
    assert_non_null (schema_context);

    // Invalid input check
    status = dc_get_elements (NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_elements (schema_context, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_elements (NULL, &collection);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // enumerate all DISIR_CONTEST_* and attempt to get elements from invalid context type.
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);
    assert_non_null (invalid);
    while (invalid->cx_type != DISIR_CONTEXT_UNKNOWN)
    {
        // valid types
        if (invalid->cx_type == DISIR_CONTEXT_CONFIG ||
            invalid->cx_type == DISIR_CONTEXT_SCHEMA ||
            invalid->cx_type == DISIR_CONTEXT_SECTION)
        {
            invalid->cx_type++;
            continue;
        }

        status = dc_get_elements (invalid, &collection);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }

    // Valid query from schema
    status = dc_get_elements (schema_context, &collection);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_int_equal (dc_collection_size (collection), 4);

    // TODO: Query SECTION and CONFIG

    // Cleanup
    dx_context_destroy (&invalid);
    status = dc_destroy (&schema_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_collection_finished (&collection);
    assert_int_equal (status, DISIR_STATUS_OK);

    LOG_TEST_END
}

void
test_context_get_name (void **state)
{
    enum disir_status status;
    dc_t *invalid;
    dc_t *schema;
    dc_t *keyval;
    const char *name;
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (schema, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_name (keyval, "test_keyval", strlen ("test_keyval"));
    assert_int_equal (status, DISIR_STATUS_OK);


    // Invalid argument check
    status = dc_get_name (NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_name (keyval, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_name (NULL, &name, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // enumerate all DISIR_CONTEST_* and attempt to get elements from invalid context type.
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);
    assert_non_null (invalid);
    while (invalid->cx_type != DISIR_CONTEXT_UNKNOWN)
    {
        // valid types
        if (invalid->cx_type == DISIR_CONTEXT_KEYVAL ||
            invalid->cx_type == DISIR_CONTEXT_SECTION)
        {
            invalid->cx_type++;
            continue;
        }

        status = dc_get_name (invalid, &name, &size);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }

    // Test valid KEVVAL
    status = dc_get_name (keyval, &name, NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_string_equal (name, "test_keyval");

    // TODO: Test section

    // cleanup
    dx_context_destroy (&invalid);
    dc_destroy (&schema);
    dc_destroy (&keyval);

    LOG_TEST_END
}

void
test_context_get_documentation (void **state)
{
    enum disir_status status;
    dc_t *schema;
    dc_t *invalid;
    const char *doc;
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_documentation (schema, "test_doc_string", strlen ("test_doc_string"));
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid argument check
    status = dc_get_documentation (NULL, NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (schema, NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, &doc, NULL );
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, NULL, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, &doc, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (schema, NULL, NULL, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // enumerate all DISIR_CONTEST_* and attempt to get documentation from invalid context type.
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);
    assert_non_null (invalid);
    while (invalid->cx_type != DISIR_CONTEXT_UNKNOWN)
    {
        // valid types
        if (invalid->cx_type == DISIR_CONTEXT_CONFIG ||
            invalid->cx_type == DISIR_CONTEXT_KEYVAL ||
            invalid->cx_type == DISIR_CONTEXT_SCHEMA ||
            invalid->cx_type == DISIR_CONTEXT_SECTION)
        {
            invalid->cx_type++;
            continue;
        }

        status = dc_get_documentation (invalid, NULL, &doc, &size);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }

    // Get valid documentation from schema
    status = dc_get_documentation (schema, NULL, &doc, &size);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_string_equal (doc, "test_doc_string");

    // TODO: Test more valid contextx

    // TODO: Corner case test the semver version getter

    // cleanup
    dx_context_destroy (&invalid);
    dc_destroy (&schema);

    LOG_TEST_END
}

const struct CMUnitTest disir_context_query_tests[] = {
    cmocka_unit_test (test_context_get_elements),
    cmocka_unit_test (test_context_get_name),
    cmocka_unit_test (test_context_get_documentation),
};

