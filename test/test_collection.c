
#include "collection.h"

void test_collection_basic(void **state)
{
    enum disir_status status;
    dcc_t *collection;
    int32_t size;

    LOG_TEST_START

    collection = dx_collection_create();
    assert_non_null(collection);

    // *_size invalid param
    size = dc_collection_size(NULL);
    assert_int_equal(size, 0);

    size = dc_collection_size(collection);
    assert_int_equal(size, 0);

    status = dc_collection_finished(NULL);
    assert_int_equal(status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dc_collection_finished(&collection);
    assert_int_equal(status, DISIR_STATUS_OK);
    assert_null(collection);

    LOG_TEST_END
}

void test_collection_add_to_trigger_resize(void **state)
{
    enum disir_status status;
    dcc_t *collection;
    dc_t *contexts[255];
    uint32_t i;
    int32_t size;

    LOG_TEST_START

    collection = dx_collection_create();
    assert_non_null(collection);

    for (i = 0; i < 255; i++)
    {
        contexts[i] = dx_context_create(DISIR_CONTEXT_DOCUMENTATION);
        status = dx_collection_push_context(collection, contexts[i]);
        assert_int_equal(status, DISIR_STATUS_OK);
        size = dc_collection_size(collection);
        assert_int_equal(size, i + 1);

    }

    for (i = 0; i < 255; i++)
    {
        status = dc_destroy(&contexts[i]);
        // status is not OK if the context contains no value.
        size = dc_collection_size(collection);
        assert_int_equal(size, 255 - (i + 1));
    }

    status = dc_collection_finished(&collection);
    assert_int_equal(status, DISIR_STATUS_OK);
    assert_null(collection);

    LOG_TEST_END
}

void test_collection_next(void **state)
{
    enum disir_status status;
    dcc_t *collection;
    dc_t *contexts[100];
    dc_t *current;
    uint32_t i;

    LOG_TEST_START

    collection = dx_collection_create();
    assert_non_null(collection);

    // Insert 100 entries
    for (i = 0; i < 100; i++)
    {
        contexts[i] = dx_context_create(DISIR_CONTEXT_DOCUMENTATION);
        status = dx_collection_push_context(collection, contexts[i]);
        assert_int_equal(status, DISIR_STATUS_OK);
    }

    // Iterate through all of them
    for (i = 0; i < 100; i++)
    {
        status = dc_collection_next(collection, &current);
        assert_int_equal(status, DISIR_STATUS_OK);
        assert_ptr_equal(current, contexts[i]);
    }

    // Iterator should be exhausted now
    status = dc_collection_next(collection, &current);
    assert_int_equal(status, DISIR_STATUS_EXHAUSTED);

    // Reset iterator
    status = dc_collection_reset(collection);
    assert_int_equal(status, DISIR_STATUS_OK);

    for (i = 0; i < 100; i++)
    {
        log_test("Context at index( %d ): %p", i, collection->cc_collection[i]);
    }


    // Iterate 25 - destroy 10 before and 10 after
    // Iterate til end.
    // The internal iterator index should still return the correct entry, 
    for (i = 0; i < 25; i++)
    {
        status = dc_collection_next(collection, &current);
        assert_int_equal(status, DISIR_STATUS_OK);
        assert_ptr_equal(current, contexts[i]);
    }
    for (i = 0; i <= 20; i += 2)
    {
        dc_destroy(&contexts[i]);
    }
    for (i = 50; i < 80; i += 3)
    {
        dc_destroy(&contexts[i]);
    }

    for (i = 25; i < 100; i++)
    {
        if (contexts[i] == NULL)
        {
            continue;
        }

        status = dc_collection_next(collection, &current);
        assert_int_equal(status, DISIR_STATUS_OK);
        assert_ptr_equal(current, contexts[i]);
    }

    status = dc_collection_finished(&collection);
    assert_int_equal(status, DISIR_STATUS_OK);
    assert_null(collection);

    LOG_TEST_END
}

void test_collection_add_multiple(void **state)
{
    enum disir_status status;
    dcc_t *collection;
    uint32_t i;
    int32_t size;
    dc_t *contexts[20];

    LOG_TEST_START

    collection = dx_collection_create();
    assert_non_null(collection);

    for (i =  0; i < 10; i++)
    {
        // QUESTION: Can we safely only allocate a context, without allocating the
        // corresponding value? :s
        contexts[i] = dx_context_create(DISIR_CONTEXT_DOCUMENTATION);
        status = dx_collection_push_context(collection, contexts[i]);
        assert_int_equal(status, DISIR_STATUS_OK);
    }

    // Destroy a few of them
    dc_destroy(&contexts[2]);
    dc_destroy(&contexts[6]);
    dc_destroy(&contexts[9]);

    // Add 5 more
    for (; i < 15; i++)
    {
        contexts[i] = dx_context_create(DISIR_CONTEXT_DOCUMENTATION);
        status = dx_collection_push_context(collection, contexts[i]);
        assert_int_equal(status, DISIR_STATUS_OK);
    }

    size = dc_collection_size(collection);
    assert_int_equal(size, 12);

    status = dc_collection_finished(&collection);
    assert_int_equal(status, DISIR_STATUS_OK);
    assert_null(collection);

    LOG_TEST_END
}

const struct CMUnitTest disir_collection_tests[] = {
    cmocka_unit_test(test_collection_basic),
    cmocka_unit_test(test_collection_add_multiple),
    cmocka_unit_test(test_collection_add_to_trigger_resize),
    cmocka_unit_test(test_collection_next),
};

