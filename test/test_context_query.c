
#include <disir/disir.h>
#include <disir/collection.h>

void
test_context_get_elements (void **state)
{
    enum disir_status status;
    dc_t *invalid;
    dc_t *mold_context;
    dcc_t *collection;
    struct disir_mold *mold;

    LOG_TEST_START

    // Setup mold
    status = dc_mold_begin (&mold_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (mold_context, "keyval1",
                                   "keyval1_value", "keyval1_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (mold_context, "keyval2", "keyval2_value",
                                   "keyval2_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (mold_context, "keyval3", "keyval3_value",
                                   "keyval3_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_keyval_string (mold_context, "keyval4", "keyval4_value",
                                   "keyval4_doc", NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_mold_finalize (&mold_context, &mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    mold_context = dc_mold_getcontext (mold);
    assert_non_null (mold_context);

    // Invalid input check
    status = dc_get_elements (NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_elements (mold_context, NULL);
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
            invalid->cx_type == DISIR_CONTEXT_MOLD ||
            invalid->cx_type == DISIR_CONTEXT_SECTION)
        {
            invalid->cx_type++;
            continue;
        }

        status = dc_get_elements (invalid, &collection);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }

    // Valid query from mold
    status = dc_get_elements (mold_context, &collection);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_int_equal (dc_collection_size (collection), 4);

    // TODO: Query SECTION and CONFIG

    // Cleanup
    dx_context_destroy (&invalid);
    status = dc_destroy (&mold_context);
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
    dc_t *mold;
    dc_t *keyval;
    const char *name;
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_mold_begin (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (mold, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_set_name (keyval, "test_keyval", strlen ("test_keyval"));
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
    dc_destroy (&mold);
    dc_destroy (&keyval);

    LOG_TEST_END
}

void
test_context_get_documentation (void **state)
{
    enum disir_status status;
    dc_t *mold;
    dc_t *invalid;
    const char *doc;
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_mold_begin (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_documentation (mold, "test_doc_string", strlen ("test_doc_string"));
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid argument check
    status = dc_get_documentation (NULL, NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (mold, NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, &doc, NULL );
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, NULL, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (NULL, NULL, &doc, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_get_documentation (mold, NULL, NULL, &size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // enumerate all DISIR_CONTEST_* and attempt to get documentation from invalid context type.
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);
    assert_non_null (invalid);
    while (invalid->cx_type != DISIR_CONTEXT_UNKNOWN)
    {
        // valid types
        if (invalid->cx_type == DISIR_CONTEXT_CONFIG ||
            invalid->cx_type == DISIR_CONTEXT_KEYVAL ||
            invalid->cx_type == DISIR_CONTEXT_MOLD ||
            invalid->cx_type == DISIR_CONTEXT_SECTION)
        {
            invalid->cx_type++;
            continue;
        }

        status = dc_get_documentation (invalid, NULL, &doc, &size);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }

    // Get valid documentation from mold
    status = dc_get_documentation (mold, NULL, &doc, &size);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_string_equal (doc, "test_doc_string");

    // TODO: Test more valid contextx

    // TODO: Corner case test the semver version getter

    // cleanup
    dx_context_destroy (&invalid);
    dc_destroy (&mold);

    LOG_TEST_END
}

const struct CMUnitTest disir_context_query_tests[] = {
    cmocka_unit_test (test_context_get_elements),
    cmocka_unit_test (test_context_get_name),
    cmocka_unit_test (test_context_get_documentation),
};

