
#include <stdio.h>

#include "context_private.h"
#include "config.h"
#include "mold.h"

int setup_context_config(void **state)
{
    dc_t *config;

    config = dx_context_create(DISIR_CONTEXT_CONFIG);
    if (config == NULL)
        return -1;

    // Allocate internal disir_documentation
    config->cx_config = dx_config_create(config);
    if (config->cx_config == NULL)
        return -1;

    *state = config;
    return 0;
}

int teardown_context_config(void **state)
{
    dc_t *config;

    config = *state;

    dx_config_destroy(&config->cx_config);

    return 0;
}

int setup_context_mold(void **state)
{
    dc_t *mold;

    mold = dx_context_create(DISIR_CONTEXT_MOLD);
    if (mold== NULL)
        return -1;

    // Allocate internal disir_documentation
    mold->cx_mold = dx_mold_create(mold);
    if (mold->cx_mold == NULL)
        return -1;

    *state = mold;
    return 0;
}

int teardown_context_mold(void **state)
{
    dc_t *mold;

    mold= *state;

    dx_mold_destroy(&mold->cx_mold);

    return 0;
}

static void
test_context_create(void **state)
{
    dc_t *context;

    LOG_TEST_START

    context = dx_context_create(DISIR_CONTEXT_CONFIG);
    assert_non_null(context);

    assert_int_equal(context->cx_refcount, 1);

    LOG_TEST_END
}

static void
test_context_destroy(void **state)
{
    dc_t *context;

    LOG_TEST_START

    // prep
    context = dx_context_create(DISIR_CONTEXT_CONFIG);
    assert_non_null(context);

    dc_destroy(&context);
    assert_null(context);

    LOG_TEST_END
}


// Test the allocator for each context type, and
// query that dc_type returns the correct type.
// Verify that the correct type returns the correct string representation.
static void
test_context_type(void **state)
{
    dc_t c;
    dc_t *context = &c;

    LOG_TEST_START

    enum disir_context_type type;

    for (type = DISIR_CONTEXT_CONFIG; type <= DISIR_CONTEXT_UNKNOWN; type++)
    {
        context->cx_type = type;
        switch (type)
        {
        case DISIR_CONTEXT_CONFIG:
            assert_true(dc_type(context) == DISIR_CONTEXT_CONFIG);
            assert_string_equal(dc_type_string(context), "FILE_CONFIG");
            break;
        case DISIR_CONTEXT_MOLD:
            assert_true(dc_type(context) == DISIR_CONTEXT_MOLD);
            assert_string_equal(dc_type_string(context), "FILE_MOLD");
            break;
        case DISIR_CONTEXT_SECTION:
            assert_true(dc_type(context) == DISIR_CONTEXT_SECTION);
            assert_string_equal(dc_type_string(context), "SECTION");
            break;
        case DISIR_CONTEXT_DOCUMENTATION:
            assert_true(dc_type(context) == DISIR_CONTEXT_DOCUMENTATION);
            assert_string_equal(dc_type_string(context), "DOCUMENTATION");
            break;
        case DISIR_CONTEXT_KEYVAL:
            assert_true(dc_type(context) == DISIR_CONTEXT_KEYVAL);
            assert_string_equal(dc_type_string(context), "KEYVAL");
            break;
        case DISIR_CONTEXT_DEFAULT:
            assert_true(dc_type(context) == DISIR_CONTEXT_DEFAULT);
            assert_string_equal(dc_type_string(context), "DEFAULT");
            break;
        case DISIR_CONTEXT_RESTRICTION:
            assert_true(dc_type(context) == DISIR_CONTEXT_RESTRICTION);
            assert_string_equal(dc_type_string(context), "RESTRICTION");
            break;
        case DISIR_CONTEXT_UNKNOWN:
            assert_true(dc_type(context) == DISIR_CONTEXT_UNKNOWN);
            assert_string_equal(dc_type_string(context), "UNKNOWN");
            break;
         // No default entry - let compiler warn us if we have not handled a case.
        }
    }

    //! Zero value
    c.cx_type = 0;
    assert_true(dc_type(context) == DISIR_CONTEXT_UNKNOWN);
    assert_string_equal(dc_type_string(context), "UNKNOWN");

    //! Negative value
    c.cx_type = -5;
    assert_true(dc_type(context) == DISIR_CONTEXT_UNKNOWN);
    assert_string_equal(dc_type_string(context), "UNKNOWN");

    // Random waay to high value
    c.cx_type = 87634;
    assert_true(dc_type(context) == DISIR_CONTEXT_UNKNOWN);
    assert_string_equal(dc_type_string(context), "UNKNOWN");

    LOG_TEST_END
}

void test_context_type_check(void **state)
{
    dc_t context;
    enum disir_status status;

    LOG_TEST_START

    // input invalid context less than zero.
    context.cx_type = -5;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG);
    assert_int_equal(status, DISIR_STATUS_INVALID_CONTEXT);

    // Input invalid context of way too high number
    context.cx_type = 98734;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG);
    assert_int_equal(status, DISIR_STATUS_INVALID_CONTEXT);

    // Input context type of zero
    context.cx_type = 0;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG);
    assert_int_equal(status, DISIR_STATUS_INVALID_CONTEXT);

    // Valid context, but called with no set to check against.
    // Apperantly, CONTEXT_TYPE_CHECK(&context) wont even compile
    //  due to __VA_ARGS__ macro expansion. Attempt to called
    //  underlying function directly. Ommitting the trailing zero
    //  leads to undefined behaviour.
    context.cx_type = DISIR_CONTEXT_CONFIG;
    status = dx_context_type_check_log_error(&context, 0);
    assert_int_equal(status, DISIR_STATUS_TOO_FEW_ARGUMENTS);

    // Valid context, against single incorrect, valid context.
    context.cx_type = DISIR_CONTEXT_MOLD;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG);
    assert_int_equal(status, DISIR_STATUS_WRONG_CONTEXT);

    // Valid context against multiple incorrect, valid contexts.
    context.cx_type = DISIR_CONTEXT_MOLD;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG,
                                DISIR_CONTEXT_DEFAULT, DISIR_CONTEXT_SECTION);
    assert_int_equal(status, DISIR_STATUS_WRONG_CONTEXT);

    // Valid context, against correct single valid context
    context.cx_type = DISIR_CONTEXT_MOLD;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_MOLD);
    assert_int_equal(status, DISIR_STATUS_OK);

    // Valid context against multiple correct, valid contexts.
    context.cx_type = DISIR_CONTEXT_SECTION;
    status = CONTEXT_TYPE_CHECK(&context, DISIR_CONTEXT_CONFIG,
                                DISIR_CONTEXT_DEFAULT, DISIR_CONTEXT_SECTION);
    assert_int_equal(status, DISIR_STATUS_OK);

    LOG_TEST_END
}

void
test_context_value_type (void **state)
{
    enum disir_value_type type;

    LOG_TEST_START

    for (type = DISIR_VALUE_TYPE_STRING; type <= DISIR_VALUE_TYPE_UNKNOWN; type++)
    {
        switch (type)
        {
        case DISIR_VALUE_TYPE_STRING:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_STRING);
            assert_string_equal (dx_value_type_string (type), "STRING");
            break;
        case DISIR_VALUE_TYPE_INTEGER:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_INTEGER);
            assert_string_equal (dx_value_type_string (type), "INTEGER");
            break;
        case DISIR_VALUE_TYPE_FLOAT:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_FLOAT);
            assert_string_equal (dx_value_type_string (type), "FLOAT");
            break;
        case DISIR_VALUE_TYPE_BOOLEAN:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_BOOLEAN);
            assert_string_equal (dx_value_type_string (type), "BOOLEAN");
            break;
        case DISIR_VALUE_TYPE_ENUM:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_ENUM);
            assert_string_equal (dx_value_type_string (type), "ENUM");
            break;
        case DISIR_VALUE_TYPE_UNKNOWN:
            assert_true (dx_value_type_sanify (type) == DISIR_VALUE_TYPE_UNKNOWN);
            assert_string_equal (dx_value_type_string (type), "UNKNOWN");
            break;
         // No default entry - let compiler warn us if we have not handled a case.
        }
    }

    //! out of bounds values
    assert_true (dx_value_type_sanify (0) == DISIR_VALUE_TYPE_UNKNOWN);
    assert_true (dx_value_type_sanify (-1) == DISIR_VALUE_TYPE_UNKNOWN);
    assert_true (dx_value_type_sanify (659843) == DISIR_VALUE_TYPE_UNKNOWN);

    LOG_TEST_END
}


const struct CMUnitTest disir_context_tests[] = {
  cmocka_unit_test(test_context_type),
  cmocka_unit_test(test_context_type_check),
  cmocka_unit_test(test_context_create),
  cmocka_unit_test(test_context_destroy),
  cmocka_unit_test(test_context_value_type),
};
