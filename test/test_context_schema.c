
#include <string.h>

#include "schema.h"
#include "keyval.h"

void
test_context_schema_add_keyval (void **state)
{
    enum disir_status status;
    dc_t *schema;
    dc_t *keyval;
    int32_t size;

    keyval = NULL;
    schema = NULL;

    LOG_TEST_START

    // setup
    status =  dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (schema);

    // Assert element storage is empty
    size = dx_element_storage_numentries (schema->cx_schema->sc_elements);
    assert_int_equal (size, 0);

    // Add keyval
    status = dc_begin (schema, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (keyval);
    // Add name
    status = dc_add_name (keyval, "test_string", strlen ("test_string")),
    assert_int_equal (status, DISIR_STATUS_OK);
    // Add type
    status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_STRING),
    assert_int_equal (status, DISIR_STATUS_OK);
    // Add default
    status = dc_add_default_string (keyval, "TNT", strlen ("TNT"), NULL);
    assert_int_equal (status, DISIR_STATUS_OK);

    status = dc_finalize (&keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (keyval);

    // Assert element storage contains one entry
    size = dx_element_storage_numentries (schema->cx_schema->sc_elements);
    assert_int_equal (size, 1);

    // Cleanup
    status = dc_destroy (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (schema);

    LOG_TEST_END
}

void
test_context_schema_add_documentation (void **state)
{
    enum disir_status status;
    dc_t *schema;
    char docstring[] = "A somewhat ponderous plan.";
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (schema);
    size = strlen (docstring);

    status = dc_add_documentation (schema, docstring, size);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Assert that the docstring is present in the schema
    // TODO: Need to implement getter

    // Cleanup
    status = dc_destroy (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (schema);

    LOG_TEST_END
}

void
test_context_schema_basic (void **state)
{
    enum disir_status status;
    dc_t *schema_context;
    dc_t *context;
    struct disir_schema *schema;

    schema = NULL;
    schema_context = NULL;
    context = NULL;

    LOG_TEST_START

    // setup
    context = dx_context_create (DISIR_CONTEXT_CONFIG); // first enum in disir_context_type
    assert_non_null (context);

    schema = dx_schema_create (NULL);
    assert_non_null (schema);
    status = dx_schema_destroy (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid argument check
    status = dc_schema_begin (NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Valid check
    status = dc_schema_begin (&schema_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (schema_context);

    // Destroy check
    status = dc_destroy (&schema_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (schema_context);

    // Finalize check
    status = dc_schema_begin (&schema_context);
    schema = NULL;
    // invalid check
    status = dc_schema_finalize (&schema_context, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_schema_finalize (NULL, &schema);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    // Check every other context type except schema as input
    while (dc_type (context) != DISIR_CONTEXT_UNKNOWN)
    {
        if (dc_type (context) == DISIR_CONTEXT_SCHEMA)
        {
            context->cx_type++;
            continue;
        }

        status = dc_schema_finalize (&context, &schema);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);
        context->cx_type++;
    }

    // valid check
    status = dc_schema_finalize (&schema_context, &schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (schema);
    assert_null (schema_context);

    // getcontext
    schema_context = dc_schema_getcontext (NULL);
    assert_null (schema_context);
    schema_context = dc_schema_getcontext (schema);
    assert_non_null (schema_context);
    assert_int_equal (dc_type (schema_context), DISIR_CONTEXT_SCHEMA);

    LOG_TEST_END
}

void
test_context_schema_version (void **state)
{
    enum disir_status status;
    struct disir_schema *schema;
    dc_t *context;
    struct semantic_version input;
    struct semantic_version output;

    LOG_TEST_START

    // setup
    status = dc_schema_begin (&context);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_schema_finalize (&context, &schema);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Get initialized context at 1.0.0
    input.sv_major = 1;
    input.sv_minor = 0;
    input.sv_patch = 0;
    status = dc_schema_get_version (schema, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update version (with internal function)
    input.sv_minor = 4;
    status = dx_schema_update_version (schema, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert schema got updated to input
    status = dc_schema_get_version (schema, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update all three
    input.sv_major = 3;
    input.sv_minor = 1;
    input.sv_patch = 10;
    status = dx_schema_update_version (schema, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert schema got updated to input
    status = dc_schema_get_version (schema, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update with lower semver
    input.sv_major = 2;
    input.sv_minor = 7;
    input.sv_patch = 3;
    status = dx_schema_update_version (schema, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert schema did not update version
    status = dc_schema_get_version (schema, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) < 0);

    LOG_TEST_END
}

const struct CMUnitTest disir_context_schema_tests[] = {
    cmocka_unit_test (test_context_schema_basic),
    cmocka_unit_test (test_context_schema_add_documentation),
    cmocka_unit_test (test_context_schema_add_keyval),
    cmocka_unit_test (test_context_schema_version),
};

