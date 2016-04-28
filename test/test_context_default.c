
#include "default.h"
#include "keyval.h"

void
test_context_default_basic (void **state)
{
    enum disir_status status;
    struct disir_default *def;
    dc_t *context;
    dc_t *schema;
    dc_t *keyval;
    dc_t *invalid;

    def = NULL;

    LOG_TEST_START

    // Setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (schema, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    invalid = dx_context_create (DISIR_CONTEXT_CONFIG);

    // Assert create and destroy methods
    def = dx_default_create (NULL);
    assert_non_null (def);
    status = dx_default_destroy (&def);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (def);

    // Invalid default_begin input
    status = dx_default_begin (NULL, &context);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_default_begin (keyval, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // enumerate all DISIR_CONTEST_* and attempt to add default with invalid parent.
    invalid->cx_type = DISIR_CONTEXT_CONFIG;
    while (invalid->cx_type != DISIR_CONTEXT_UNKNOWN)
    {
        // valid types
        if (invalid->cx_type == DISIR_CONTEXT_KEYVAL)
        {
            invalid->cx_type++;
            continue;
        }

        status = dx_default_begin (invalid, &context);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);

        invalid->cx_type++;
    }


    // Begin default on a keyval without a type shall fail
    status = dc_begin (keyval, DISIR_CONTEXT_DEFAULT, &context);
    assert_int_equal (status, DISIR_STATUS_INVALID_CONTEXT);

    // Begin default on a keyval with a type shall succeed
    status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_STRING);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (keyval, DISIR_CONTEXT_DEFAULT, &context);
    assert_int_equal (status, DISIR_STATUS_OK);


    status = dc_destroy (&keyval);
    assert_int_equal (status, DISIR_STATUS_OK);

    // cleanup
    dx_context_destroy (&invalid);
    dc_destroy (&schema);

    LOG_TEST_END
}

void
test_context_default_integer (void **state)
{
    enum disir_status status;
    dc_t *keyval;
    dc_t *schema;
    struct semantic_version semver;

    semver.sv_major = 1;
    semver.sv_minor = 0;
    semver.sv_patch = 0;

    LOG_TEST_START

    // Setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (schema, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_INTEGER);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid arument
    status = dc_add_default_integer (NULL, 0, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Valid argument
    status = dc_add_default_integer (keyval, 10, NULL);
    assert_int_equal (status, DISIR_STATUS_OK);

    semver.sv_major = 2;

    // Invoke generic add method on integer vallue type keyval
    status = dc_add_default (keyval, "", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dc_add_default (keyval, "string start 50123", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dc_add_default (keyval, "66439", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_OK);


    LOG_TEST_END
}

void
test_context_default_float (void **state)
{
    enum disir_status status;
    dc_t *keyval;
    dc_t *schema;
    struct semantic_version semver;

    semver.sv_major = 1;
    semver.sv_minor = 0;
    semver.sv_patch = 0;

    LOG_TEST_START

    // Setup
    status = dc_schema_begin (&schema);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_begin (schema, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_set_value_type (keyval, DISIR_VALUE_TYPE_FLOAT);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid arument
    status = dc_add_default_float (NULL, 0, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Valid argument
    status = dc_add_default_float (keyval, 10, NULL);
    assert_int_equal (status, DISIR_STATUS_OK);

    semver.sv_major = 2;

    // Invoke generic add method on integer vallue type keyval
    status = dc_add_default (keyval, "", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dc_add_default (keyval, "string start 50123", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dc_add_default (keyval, "3.14", 1000, &semver);
    assert_int_equal (status, DISIR_STATUS_OK);


    LOG_TEST_END
}

const struct CMUnitTest disir_context_default_tests[] = {
    cmocka_unit_test (test_context_default_basic),
    cmocka_unit_test (test_context_default_integer),
    cmocka_unit_test (test_context_default_float),
};

