#include <disir/util.h>

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

const struct CMUnitTest disir_util_tests[] = {
    // introduced can add
    cmocka_unit_test (test_semantic_version_compare)
};

