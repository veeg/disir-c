
void
test_introduced_context_can_add(void **state)
{
    dc_t *parent = *state;
    int res;
    enum disir_status status;
    struct semantic_version semver_input;
    struct semantic_version semver_output;

    LOG_TEST_CONTEXT_START(parent);

    semver_input.sv_major = 4;
    semver_input.sv_minor = 5;
    semver_input.sv_patch = 99;

    // Context shall be allowed to add introduced
    status = dc_add_introduced(parent, semver_input);
    assert_int_equal(status, DISIR_STATUS_OK);

    // Get introduced value - should be equal to input
    status = dc_get_introduced(parent, &semver_output);
    assert_int_equal(status, DISIR_STATUS_OK);
    res = dx_semantic_version_compare(&semver_input, &semver_output);
    assert_int_equal(res, 0);

    LOG_TEST_CONTEXT_END(parent);
}

void
test_introduced_context_cannot_add(void **state)
{
    dc_t *parent = *state;
    enum disir_status status;
    struct semantic_version semver_input;
    struct semantic_version semver_output;

    LOG_TEST_CONTEXT_START(parent);

    semver_input.sv_major = 4;
    semver_input.sv_minor = 5;
    semver_input.sv_patch = 99;

    // Context shall not be allowed to add introduced
    status = dc_add_introduced(parent, semver_input);
    assert_int_equal(status, DISIR_STATUS_NO_CAN_DO);

    // Attempt to get introduced - shall not be allowed
    status = dc_get_introduced(parent, &semver_output);
    assert_int_equal(status, DISIR_STATUS_NO_CAN_DO);

    LOG_TEST_CONTEXT_END(parent);
}

const struct CMUnitTest disir_introduced_tests[] = {
    // introduced can add
    cmocka_unit_test_setup_teardown(
        test_introduced_context_can_add,
        setup_context_documentation, teardown_context_documentation),
    // introduced cannot add
    cmocka_unit_test_setup_teardown(
        test_introduced_context_cannot_add,
        setup_context_config, teardown_context_config),
};
