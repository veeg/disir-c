#include <disir/util.h>
#include "value.h"

void
test_semantic_version_compare(void **state)
{
    int res;
    struct semantic_version sv1;
    struct semantic_version sv2;

    sv1.sv_major = sv2.sv_major = 3;
    sv1.sv_minor = sv2.sv_minor = 6;
    sv1.sv_patch = sv2.sv_patch = 76;

    LOG_TEST_START

    // Equality
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_int_equal (res, 0);

    // sv1 major greater
    sv1.sv_major = 56;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res > 0);

    // sv1 major less
    sv1.sv_major = 1;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res < 0);


    // restore sv1
    sv1.sv_major = 3;

    // sv1 minor greater
    sv1.sv_minor = 89;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res > 0);

    // sv1 minor less
    sv1.sv_minor = 4;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res < 0);

    // restore sv1
    sv1.sv_minor = 6;

    // sv1 patch greater
    sv1.sv_patch = 105;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res > 0);

    // sv1 patch less
    sv1.sv_patch = 56;
    res = dx_semantic_version_compare (&sv1, &sv2);
    assert_true (res < 0);


    LOG_TEST_END
}

static void
test_value_string (void **state)
{
    enum disir_status status;
    struct disir_value value;
    const char *test_string = "A explaination of everything good about this product.";
    int32_t string_length;
    int32_t output_size;
    const char *output_string;

    LOG_TEST_START

    string_length = strlen (test_string);

    // Invalid argument check
    status = dx_value_set_string (NULL, NULL, 0);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_value_set_string (NULL, test_string, 0);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Invalid type
    value.dv_type = DISIR_VALUE_TYPE_UNKNOWN;
    status = dx_value_set_string (&value, test_string, string_length);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Valid type
    value.dv_type = DISIR_VALUE_TYPE_STRING;
    status = dx_value_set_string (&value, test_string, string_length);
    assert_int_equal (status, DISIR_STATUS_OK);

    assert_int_equal (value.dv_size, string_length);
    assert_memory_equal (test_string, value.dv_string, string_length);

    // Test getter
    status = dx_value_get_string (NULL, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_value_get_string (&value, NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_value_get_string (NULL, &output_string, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dx_value_get_string (&value, &output_string, NULL);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dx_value_get_string (&value, &output_string, &output_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    assert_int_equal (string_length, output_size);
    assert_memory_equal (output_string, test_string, string_length);

    // Set NULL
    status = dx_value_set_string (&value, NULL, 0);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_int_equal (value.dv_size, 0);
    assert_null (value.dv_string);

    status = dx_value_get_string (&value, &output_string, &output_size);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_int_equal (output_size, 0);
    assert_null (output_string);

    LOG_TEST_END
}

const struct CMUnitTest disir_util_tests[] = {
    // introduced can add
    cmocka_unit_test (test_semantic_version_compare),
    cmocka_unit_test (test_value_string),
};

