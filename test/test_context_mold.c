
#include <string.h>

#include "mold.h"
#include "keyval.h"
#include "util_private.h"

void
test_context_mold_add_keyval (void **state)
{
    enum disir_status status;
    dc_t *mold;
    dc_t *keyval;
    int32_t size;

    keyval = NULL;
    mold = NULL;

    LOG_TEST_START

    // setup
    status =  dc_mold_begin (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (mold);

    // Assert element storage is empty
    size = dx_element_storage_numentries (mold->cx_mold->mo_elements);
    assert_int_equal (size, 0);

    // Add keyval
    status = dc_begin (mold, DISIR_CONTEXT_KEYVAL, &keyval);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (keyval);
    // Add name
    status = dc_set_name (keyval, "test_string", strlen ("test_string")),
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
    size = dx_element_storage_numentries (mold->cx_mold->mo_elements);
    assert_int_equal (size, 1);

    // Cleanup
    status = dc_destroy (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (mold);

    LOG_TEST_END
}

void
test_context_mold_add_documentation (void **state)
{
    enum disir_status status;
    dc_t *mold;
    char docstring[] = "A somewhat ponderous plan.";
    int32_t size;

    LOG_TEST_START

    // setup
    status = dc_mold_begin (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (mold);
    size = strlen (docstring);

    status = dc_add_documentation (mold, docstring, size);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Assert that the docstring is present in the mold
    // TODO: Need to implement getter

    // Cleanup
    status = dc_destroy (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (mold);

    LOG_TEST_END
}

void
test_context_mold_basic (void **state)
{
    enum disir_status status;
    dc_t *mold_context;
    dc_t *context;
    struct disir_mold *mold;

    mold = NULL;
    mold_context = NULL;
    context = NULL;

    LOG_TEST_START

    // setup
    context = dx_context_create (DISIR_CONTEXT_CONFIG); // first enum in disir_context_type
    assert_non_null (context);

    mold = dx_mold_create (NULL);
    assert_non_null (mold);
    status = dx_mold_destroy (&mold);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Invalid argument check
    status = dc_mold_begin (NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Valid check
    status = dc_mold_begin (&mold_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (mold_context);

    // Destroy check
    status = dc_destroy (&mold_context);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_null (mold_context);

    // Finalize check
    status = dc_mold_begin (&mold_context);
    mold = NULL;
    // invalid check
    status = dc_mold_finalize (&mold_context, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_mold_finalize (NULL, &mold);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    // Check every other context type except mold as input
    while (dc_type (context) != DISIR_CONTEXT_UNKNOWN)
    {
        if (dc_type (context) == DISIR_CONTEXT_MOLD)
        {
            context->cx_type++;
            continue;
        }

        status = dc_mold_finalize (&context, &mold);
        assert_int_equal (status, DISIR_STATUS_WRONG_CONTEXT);
        context->cx_type++;
    }

    // valid check
    status = dc_mold_finalize (&mold_context, &mold);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (mold);
    assert_null (mold_context);

    // getcontext
    mold_context = dc_mold_getcontext (NULL);
    assert_null (mold_context);
    mold_context = dc_mold_getcontext (mold);
    assert_non_null (mold_context);
    assert_int_equal (dc_type (mold_context), DISIR_CONTEXT_MOLD);

    LOG_TEST_END
}

void
test_context_mold_version (void **state)
{
    enum disir_status status;
    struct disir_mold *mold;
    dc_t *context;
    struct semantic_version input;
    struct semantic_version output;

    LOG_TEST_START

    // setup
    status = dc_mold_begin (&context);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_mold_finalize (&context, &mold);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Get initialized context at 1.0.0
    input.sv_major = 1;
    input.sv_minor = 0;
    input.sv_patch = 0;
    status = dc_mold_get_version (mold, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update version (with internal function)
    input.sv_minor = 4;
    status = dx_mold_update_version (mold, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert mold got updated to input
    status = dc_mold_get_version (mold, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update all three
    input.sv_major = 3;
    input.sv_minor = 1;
    input.sv_patch = 10;
    status = dx_mold_update_version (mold, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert mold got updated to input
    status = dc_mold_get_version (mold, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) == 0);

    // Update with lower semver
    input.sv_major = 2;
    input.sv_minor = 7;
    input.sv_patch = 3;
    status = dx_mold_update_version (mold, &input);
    assert_int_equal (status, DISIR_STATUS_OK);
    // Assert mold did not update version
    status = dc_mold_get_version (mold, &output);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_true (dx_semantic_version_compare (&input, &output) < 0);

    LOG_TEST_END
}

const struct CMUnitTest disir_context_mold_tests[] = {
    cmocka_unit_test (test_context_mold_basic),
    cmocka_unit_test (test_context_mold_add_documentation),
    cmocka_unit_test (test_context_mold_add_keyval),
    cmocka_unit_test (test_context_mold_version),
};

