#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <string.h>

#include "multimap.h"

const char *keys[] = {
    "Hammoc", 
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
    NULL
};

// http://www.cse.yorku.ca/~oz/hash.html
unsigned long djb2(char *str) {
	unsigned long hash = 5381;
	char c;
	while( (c = *str++) ) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

int
setup_multimap(void **state)
{ struct multimap *map;

    map = multimap_create ((int (*)(const void *, const void*)) strcmp, 
                           (unsigned long (*)(const void*)) djb2);

    if (map == NULL)
        return (-1);

    *state = map;

    return (0);
}

int
teardown_multimap(void **state)
{
    if (*state == NULL)
        return (-1);

    multimap_destroy ((struct multimap *)*state, NULL, NULL);

    return (0);
}

static void
test_multimap_basic (void **state)
{
    struct multimap *map;

    map = multimap_create ((int (*)(const void *, const void*)) strcmp, 
                           (unsigned long (*)(const void*)) djb2);
    assert_non_null (map);

    multimap_destroy (NULL, NULL, NULL);

    multimap_destroy (map, NULL, NULL);
}

static void
test_multimap_insert_nonduplicate_keys(void **state)
{
    
    struct multimap *map;
    const char *key;
    int result;
    int i;
    void *value;

    map = *state;

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        result = multimap_push_value (map, (void *) key, (void *) 666);
        assert_int_equal (result, 0);
    }

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        value = multimap_get_first (map, (void *) key);
        assert_int_equal ((long int) value, 666);
    }
}

static void
test_multimap_insert_duplicate_keys(void **state)
{
    
    struct multimap *map;
    const char *key;
    int result;
    int i;
    long int j;
    void *value;
    struct multimap_value_iterator *iter;

    map = *state;

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        for (j = 0; j < 10; j++)
        {
            result = multimap_push_value (map, (void *) key, (void *) j);
            assert_int_equal (result, 0);
        }
    }

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        iter = multimap_fetch (map, (void *) key);
        for (j = 0; j < 10; j++)
        {
            value = multimap_iterator_next (iter);
            assert_int_equal ((long int) value, (long int) j);
        }
        value = multimap_iterator_next (iter);
        assert_null (value);

        multimap_iterator_destroy (iter);
    }
}

static void
test_multimap_insert_duplicate_key_duplicate_value(void **state)
{
    struct multimap *map;
    const char *key;
    int result;
    int i;
    long int j;
    void *value;
    struct multimap_value_iterator *iter;

    map = *state;

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        for (j = 0; j < 10; j++)
        {
            result = multimap_push_value (map, (void *) key, (void *) j);
            assert_int_equal (result, 0);
        }

        // All duplicate values for the same key should be rejected.
        for (j = 0; j < 10; j++)
        {
            result = multimap_push_value (map, (void *) key, (void *) j);
            assert_int_equal (result, -1);
        }
    }

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        iter = multimap_fetch (map, (void *) key);
        for (j = 0; j < 10; j++)
        {
            value = multimap_iterator_next (iter);
            assert_int_equal ((long int) value, (long int) j);
        }
        value = multimap_iterator_next (iter);
        assert_null (value);

        multimap_iterator_destroy (iter);
    }
}

void
test_multimap_remove_values (void **state)
{
    struct multimap *map;
    const char *key;
    int result;
    int i;
    long int j;
    void *value;
    struct multimap_value_iterator *iter;

    map = *state;

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        for (j = 1; j < 11; j++)
        {
            result = multimap_push_value (map, (void *) key, (void *) j);
            assert_int_equal (result, 0);
        }
    }

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        value = multimap_remove_value (map, (void *) key, NULL, (void *) 10);
        assert_int_equal (value, (long int) 10);

        value = multimap_remove_value (map, (void *) key, NULL, (void *) 6);
        assert_int_equal (value, (long int) 6);

        value = multimap_remove_value (map, (void *) key, NULL, (void *) 1);
        assert_int_equal (value, (long int) 1);

    }

    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        iter = multimap_fetch (map, (void *) key);
        for (j = 1; j < 11; j++)
        {
            if (j == 1 || j == 6 || j == 10)
                continue;

            value = multimap_iterator_next (iter);
            assert_int_equal ((long int) value, (long int) j);
        }
        value = multimap_iterator_next (iter);
        assert_null (value);

        multimap_iterator_destroy (iter);
    }
}

const struct CMUnitTest multimap_tests[] = {
  cmocka_unit_test (test_multimap_basic),
  cmocka_unit_test_setup_teardown (test_multimap_insert_nonduplicate_keys, 
          setup_multimap, teardown_multimap),
  cmocka_unit_test_setup_teardown (test_multimap_insert_duplicate_keys, 
          setup_multimap, teardown_multimap),
  cmocka_unit_test_setup_teardown (test_multimap_insert_duplicate_key_duplicate_value,
          setup_multimap, teardown_multimap),
  cmocka_unit_test_setup_teardown (test_multimap_remove_values,
          setup_multimap, teardown_multimap),
};

int
main(int argc, char *argv[])
{
    int returnval = 0;

    returnval += cmocka_run_group_tests_name ("Multimap", multimap_tests, NULL, NULL);
    return returnval;
}
