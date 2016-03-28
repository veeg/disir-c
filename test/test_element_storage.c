
#include "element_storage.h"

const char *keyval_names[] = {
    "carfight",
    "snitch",
    "powertools",
    "riverbed",
    "rocks",
    "aspen",
    "thundra",
    "hellfire",
    "borean",
    "durotar",
    "thrall",
    "milk",
    "porridge",
    "flowers",
};

#define KEYVAL_NUMENTRIES (sizeof (keyval_names) / sizeof (const char *))

int
setup_element_storage (void **state)
{
    struct disir_element_storage *storage;

    storage = dx_element_storage_create ();
    if (storage == NULL)
        return (-1);

    *state = storage;

    return (0);
}

int
teardown_element_storage (void **state)
{
    struct disir_element_storage *storage;

    storage = *state;
    dx_element_storage_destroy (&storage);

    return (0);
}

static void
test_element_storage_basic(void **state)
{
    enum disir_status status;
    struct disir_element_storage *storage;
    struct disir_element_storage *null_storage;

    LOG_TEST_START

    storage = dx_element_storage_create ();
    assert_non_null (storage);

    // Invalid arguments check
    status = dx_element_storage_destroy (NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    null_storage = NULL;
    status = dx_element_storage_destroy (&null_storage);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    status = dx_element_storage_destroy (&storage);
    assert_null (storage);
    assert_int_equal (status, DISIR_STATUS_OK);

    LOG_TEST_END
}

static void
test_element_storage_add (void **state)
{
    enum disir_status status;
    dc_t *context;
    struct disir_element_storage *storage;
    const char *key;
    int i;

    storage = *state;

    LOG_TEST_START

    // Add X number of dc_t *context to storage, by name.
    for (i = 0; i < KEYVAL_NUMENTRIES; i++)
    {
        key = keyval_names[i];

        context = dx_context_create (DISIR_CONTEXT_CONFIG); // XXX: Should be DISIR_CONTEXT_KEYVAL
        assert_non_null (context);
        status = dx_element_storage_add (storage, key, context);
        assert_int_equal (status, DISIR_STATUS_OK);
        // Ref count should be 2 when inserted into element storage
        assert_int_equal (context->cx_refcount, 2);

        // Decref the context, to free up the memoey again (when element storage is destroyed)
        dx_context_decref (context);
        assert_int_equal (context->cx_refcount, 1);
    }

    LOG_TEST_END
}

void
test_element_storage_get_all (void **state)
{
    enum disir_status status;
    struct disir_element_storage *storage;
    struct disir_context_collection *collection;
    const char *key;
    dc_t *context;
    int i;
    int j;
    int index;
    dc_t **allocated_contexts;

    storage = *state;
    collection = NULL;
    index = 0;

    LOG_TEST_START

    allocated_contexts = calloc(3 * KEYVAL_NUMENTRIES, sizeof (dc_t *));
    assert_non_null (allocated_contexts);

    // Setup: Insert 3x keyval_names array of contexts into storage
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < KEYVAL_NUMENTRIES; i++)
        {
            key = keyval_names[i];

            context = dx_context_create (DISIR_CONTEXT_CONFIG);
            assert_non_null (context);
            status = dx_element_storage_add (storage, key, context);
            assert_int_equal (status, DISIR_STATUS_OK);

            allocated_contexts[index] = context;
            index++;
        }
    }

    // Invoke with invalid arguments
    status = dx_element_storage_get_all (NULL, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_element_storage_get_all (storage, NULL);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);
    status = dx_element_storage_get_all (NULL, &collection);
    assert_int_equal (status, DISIR_STATUS_INVALID_ARGUMENT);

    // Invoke with valid argument
    status = dx_element_storage_get_all (storage, &collection);
    assert_int_equal (status, DISIR_STATUS_OK);
    assert_non_null (collection);
    assert_int_equal ((3 * KEYVAL_NUMENTRIES), dc_collection_size (collection));

    // Iterate collection - should be insert order sorted
    index = 0;
    while (1)
    {
        status = dc_collection_next (collection, &context);
        assert_int_equal (status, DISIR_STATUS_OK);
        assert_ptr_equal (context, allocated_contexts[index]);
        index++;

        if (index == (3 * KEYVAL_NUMENTRIES))
            break;
    }

    status = dc_collection_next (collection, &context);
    assert_int_equal (status, DISIR_STATUS_EXHAUSTED);

    status = dc_collection_finished (&collection);
    assert_int_equal (status, DISIR_STATUS_OK);

    // cleanup
    for (i = 0; i < (3 * KEYVAL_NUMENTRIES); i++)
    {
        dx_context_decref (allocated_contexts[i]);
    }
    free (allocated_contexts);

    LOG_TEST_END
}


const struct CMUnitTest disir_element_storage_tests[] = {
  cmocka_unit_test(test_element_storage_basic),
  cmocka_unit_test_setup_teardown(test_element_storage_add,
          setup_element_storage, teardown_element_storage),
  cmocka_unit_test_setup_teardown(test_element_storage_get_all,
          setup_element_storage, teardown_element_storage),

};

