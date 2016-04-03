

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "context_private.h"
#include "documentation.h"

int
setup_context_documentation (void **state)
{
    dc_t *doc;

    doc = dx_context_create (DISIR_CONTEXT_DOCUMENTATION);
    if (doc == NULL)
        return -1;

    // Allocate internal disir_documentation
    doc->cx_documentation = dx_documentation_create (doc);
    if (doc->cx_documentation == NULL)
        return -1;

    *state = doc;
    return 0;
}

int
teardown_context_documentation (void **state)
{
    dc_t *doc;

    doc = *state;

    dx_documentation_destroy (&doc->cx_documentation);
    dx_context_destroy (&doc);

    return 0;
}

//! Test basic API functions for dx_documentation_add()
void
test_context_documentation_dx_documentation_add_basic (void **state)
{
    dc_t *parent;
    enum disir_status status;
    struct disir_documentation *doc1;
    struct disir_documentation *doc2;
    struct disir_documentation *doc3;
    struct disir_documentation *tmp;

    LOG_TEST_START

    //
    // Use DISIR_CONTEXT_CONFIG as the testbed
    // Hotfix it with more capabilities as it suits us.
    //
    status = dc_config_begin (&parent);
    assert_non_null (parent);
    assert_int_equal (status, DISIR_STATUS_OK);

    doc1 = dx_documentation_create (NULL);
    doc2 = dx_documentation_create (NULL);
    doc3 = dx_documentation_create (NULL);
    assert_non_null (doc1);
    assert_non_null (doc2);
    assert_non_null (doc3);

    // Set semver numbers
    // doc1 is middle (1.0.0)
    // doc2 is first 1.29.15
    // doc3 is last inbetween at 3.15.1
    doc1->dd_introduced.sv_major = 2;
    doc1->dd_introduced.sv_minor = 15;
    doc1->dd_introduced.sv_patch = 1;

    // Set it to initially be conflicting with doc1
    doc2->dd_introduced.sv_major = doc1->dd_introduced.sv_major;
    doc2->dd_introduced.sv_minor = doc1->dd_introduced.sv_minor;
    doc2->dd_introduced.sv_patch = doc1->dd_introduced.sv_patch;

    doc3->dd_introduced.sv_major = 3;
    doc3->dd_introduced.sv_minor = 6;
    doc3->dd_introduced.sv_patch = 2;

    // Attempt to add with NULL pointer
    status = dx_documentation_add (parent, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_documentation_add (NULL, doc1);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_documentation_add (NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Add first entry
    status = dx_documentation_add (parent, doc1);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Attempt to add an entry with conflicting semver
    status = dx_documentation_add (parent, doc2);
    assert_int_equal (status, DISIR_STATUS_CONFLICTING_SEMVER);

    // Update semver to be unique, lower than doc1
    doc2->dd_introduced.sv_major = 1;
    doc2->dd_introduced.sv_minor = 29;
    doc2->dd_introduced.sv_patch = 15;

    // Add version with lower version number
    status = dx_documentation_add (parent, doc2);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_ptr_equal (parent->cx_config->cf_documentation_queue, doc2);

    // Add last entry
    status = dx_documentation_add (parent, doc3);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Verify order in storage
    tmp = parent->cx_config->cf_documentation_queue;
    assert_ptr_equal (tmp, doc2);
    assert_non_null (tmp->next);
    tmp = tmp->next;
    assert_ptr_equal (tmp, doc1);
    assert_non_null (tmp->next);
    tmp = tmp->next;
    assert_ptr_equal (tmp, doc3);

    // This should free up all documentation entries
    // XXX: Will it tho, since its DISIR_CONTEXT_CONFIG
    // and all doc entries  has no context. Hmm
    dc_destroy (&parent);

    LOG_TEST_END
}

void
test_context_documentation_dc_add_documentation_can_add_single (void **state)
{
    dc_t *parent = *state;
    enum disir_status status;

    char doc_string[] = "A doc string that tells something";
    int32_t doc_string_size = strlen(doc_string);

    LOG_TEST_CONTEXT_START (parent);

    // Add single entry - shall succeed
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    LOG_TEST_CONTEXT_END (parent);
}

void
test_context_documentation_dc_add_documentation_cannot_add_single (void **state)
{
    dc_t *parent = *state;
    enum disir_status status;

    char doc_string[] = "A doc string that tells something";
    int32_t doc_string_size = strlen (doc_string);

    LOG_TEST_CONTEXT_START(parent);

    // Add single entry - shall fail
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_NO_CAN_DO);

    LOG_TEST_CONTEXT_END (parent);
}

void
test_context_documentation_dc_add_documentation_can_add_multiple (void **state)
{
    dc_t *parent = *state;
    enum disir_status status;

    char doc_string[] = "A doc string that tells something";
    int32_t doc_string_size = strlen (doc_string);

    LOG_TEST_CONTEXT_START (parent);

    // Add single entry - shall succeed
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Add another entry - shall succeed
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    LOG_TEST_CONTEXT_END (parent);
}

void
test_context_documentation_dc_add_documentation_cannot_add_multiple (void **state)
{
    dc_t *parent = *state;
    enum disir_status status;

    char doc_string[] = "A doc string that tells something";
    int32_t doc_string_size = strlen (doc_string);

    LOG_TEST_CONTEXT_START (parent);

    // Add single entry - shall succeed
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Add another entry - shall fail
    status = dc_add_documentation (parent, doc_string, doc_string_size);
    assert_int_equal (status, DISIR_STATUS_EXISTS);

    LOG_TEST_CONTEXT_END (parent);
}

void test_context_documentation_dc_add_documentation_basic (void **state)
{
    dc_t *parent;
    enum disir_status status;
    int32_t doc_one_size;
    char doc_string_one[] = "A doc string that tells something";

    LOG_TEST_START

    parent = NULL;
    doc_one_size = strlen (doc_string_one);

    //
    // Use DISIR_CONTEXT_CONFIG as the testbed
    // Hotfix it with more capabilities as it suits us.
    //
    status = dc_config_begin (&parent);
    assert_non_null (parent);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Test invalid parameters
    status = dc_add_documentation (NULL, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_add_documentation (parent, NULL, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_add_documentation (NULL, NULL, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_add_documentation (parent, doc_string_one, 0);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dc_add_documentation (parent, doc_string_one, -87);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Add single entry - test successful insertion
    status = dc_add_documentation (parent, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Add secondary documentation entry
    // Shall fail due to only capability to hold one documentation
    // Shall fail due to conflicting default introduced semver
    status = dc_add_documentation (parent, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_EXISTS);

    // Enable multiple documentation
    // Shall fail due to conflicting default introduced semver
    parent->CONTEXT_CAPABILITY_ADD_MULTIPLE_DOCUMENTATION = 1;
    status = dc_add_documentation (parent, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_CONFLICTING_SEMVER);

    status = dc_destroy (&parent);
    assert_int_equal (status, DISIR_STATUS_OK);

    LOG_TEST_END
}

void
test_context_documentation_dx_documentation_begin_basic (void **state)
{
    dc_t *parent;
    dc_t *doc;
    enum disir_status status;
    int32_t doc_one_size;
    char doc_string_one[] = "A doc string that tells something";
    struct semantic_version semver;

    LOG_TEST_START

    parent = NULL;
    doc = NULL;

    //
    // Use DISIR_CONTEXT_CONFIG as the testbed
    // Hotfix it with more capabilities as it suits us.
    //
    status = dc_config_begin (&parent);
    assert_non_null (parent);
    assert_int_equal (status, DISIR_STATUS_OK);

    doc_one_size = strlen (doc_string_one);
    semver.sv_major = 0;
    semver.sv_minor = 0;
    semver.sv_patch = 0;

    // Call begin with invalid arguments
    status = dx_documentation_begin (NULL, &doc);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_documentation_begin (parent, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_documentation_begin (NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Add first documentation entry with default semver
    status = dx_documentation_begin (parent, &doc);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_value_string (doc, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dx_documentation_finalize (&doc);
    assert_int_equal(status, DISIR_STATUS_OK);

    // Attempt to add another without multiple capability
    // Shall fail with EXISTS
    status = dx_documentation_begin (parent, &doc);
    assert_int_equal (status, DISIR_STATUS_EXISTS);

    // Enable multiple documentation entries
    // Add another documentation entry
    parent->CONTEXT_CAPABILITY_ADD_MULTIPLE_DOCUMENTATION = 1;
    status = dx_documentation_begin (parent, &doc);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dc_add_value_string (doc, doc_string_one, doc_one_size);
    assert_int_equal (status, DISIR_STATUS_OK);
    // same semver - fail
    status = dx_documentation_finalize (&doc);
    assert_int_equal (status, DISIR_STATUS_CONFLICTING_SEMVER);
    // add higher semver - shall succeed
    semver.sv_major = 2;
    status = dc_add_introduced (doc, semver);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dx_documentation_finalize (&doc);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Cleanup
    dc_destroy (&parent);

    LOG_TEST_END
}

void
test_context_documentation_verify_queue (void **state)
{
    enum disir_status status;
    dc_t *parent;
    dc_t *context;
    dc_t *cx_doc1, *cx_doc2, *cx_doc3;
    struct disir_documentation *doc1;
    struct disir_documentation *doc2;
    struct disir_documentation *doc3;
    struct disir_documentation *tmp;

    parent = *state;

    LOG_TEST_START

    status = dx_documentation_begin (parent, &cx_doc1);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dx_documentation_begin (parent, &cx_doc2);
    assert_int_equal (status, DISIR_STATUS_OK);
    status = dx_documentation_begin (parent, &cx_doc3);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (cx_doc1);
    assert_non_null (cx_doc2);
    assert_non_null (cx_doc3);
    doc1 = cx_doc1->cx_documentation;
    doc2 = cx_doc2->cx_documentation;
    doc3 = cx_doc3->cx_documentation;

    // Set semver numbers
    // doc1 is middle (2.15.1)
    // doc2 is first 1.29.15
    // doc3 is last inbetween at 3.6.2
    doc1->dd_introduced.sv_major = 2;
    doc1->dd_introduced.sv_minor = 15;
    doc1->dd_introduced.sv_patch = 1;

    doc2->dd_introduced.sv_major = 1;
    doc2->dd_introduced.sv_minor = 29;
    doc2->dd_introduced.sv_patch = 15;

    doc3->dd_introduced.sv_major = 3;
    doc3->dd_introduced.sv_minor = 6;
    doc3->dd_introduced.sv_patch = 2;

    // Assert that parent queue is empty when no docs are in it.
    assert_null (parent->cx_config->cf_documentation_queue);

    // Add first entry
    status = dx_documentation_add (parent, doc1);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_ptr_equal (parent->cx_config->cf_documentation_queue, doc1);

    // Add second entry that goes first in queue
    status = dx_documentation_add (parent, doc2);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_ptr_equal (parent->cx_config->cf_documentation_queue, doc2);

    // Add last entry
    status = dx_documentation_add (parent, doc3);
    assert_int_equal (status, DISIR_STATUS_OK);

    // Verify order in storage
    tmp = parent->cx_config->cf_documentation_queue;
    assert_ptr_equal (tmp, doc2);
    assert_non_null (tmp->next);
    tmp = tmp->next;
    assert_ptr_equal (tmp, doc1);
    assert_non_null (tmp->next);
    tmp = tmp->next;
    assert_ptr_equal (tmp, doc3);

    // Destroy the lowest docu entry (head of queue)
    // assert that the parent queue pointer is modified to point to the next in line
    context = doc2->dd_context;
    dc_destroy (&context);
    assert_ptr_not_equal(parent->cx_config->cf_documentation_queue, doc2);
    assert_ptr_equal (parent->cx_config->cf_documentation_queue, doc1);

    LOG_TEST_END
}

const struct CMUnitTest disir_context_documentation_tests[] = {
  cmocka_unit_test (test_context_documentation_dc_add_documentation_basic),
  cmocka_unit_test (test_context_documentation_dx_documentation_begin_basic),
  cmocka_unit_test (test_context_documentation_dx_documentation_add_basic),
    // verify queue
  cmocka_unit_test_setup_teardown (
      test_context_documentation_verify_queue,
      setup_context_config, teardown_context_config),

    // dc_add_documentation can add single
    cmocka_unit_test_setup_teardown (
        test_context_documentation_dc_add_documentation_can_add_single,
        setup_context_config, teardown_context_config),
    // dc_add_documentation cannot add single
    cmocka_unit_test_setup_teardown (
        test_context_documentation_dc_add_documentation_cannot_add_single,
        setup_context_documentation, teardown_context_documentation),
    // dc_add_documentation can add multiple
    /*
    cmocka_unit_test_setup_teardown (
        test_context_documentation_dc_add_documentation_can_add_multiple,
        setup_context_config, teardown_context_config),
    */
    // dc_add_documentation cannot add multiple
    /* Uncomment when we have context that can add single, but not multiple
    cmocka_unit_test_setup_teardown (
        test_context_documentation_dc_add_documentation_cannot_add_multiple,
        setup_context_config, teardown_context_config),
    */
};
