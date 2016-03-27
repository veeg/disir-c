
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <list.h>
#include <list_private.h>

// Helpers

#define test(fn) \
    puts("... \x1b[33m" # fn "\x1b[0m"); \
test_##fn();

char *keys[] = {
    "a",
    "b",
    "c",
    NULL,
};


static int freeProxyCalls = 0;

void
freeProxy(void *val) {
    ++freeProxyCalls;
    free(val);
}


//
// Tests
//

static void
test_list_node_new() {
    char *val = "some value";
    list_node_t *node = list_node_new(val);
    assert (node->val == val);
    free (node);
}

static void
test_list_rpush (void)
{
    int i;
    char *key;

    // Setup
    list_t *list = list_create ();

    // a b c
    for (i = 0, key = keys[i]; key != NULL; i++, key = keys[i])
    {
        list_rpush (list, key);
    }

    list_node_t *a = list->head;
    list_node_t *b = list->head->next;
    list_node_t *c = list->tail;

    // Assertions
    assert(keys[0] == list->head->val);
    assert(keys[2] == list->tail->val);
    assert(3 == list->len);
    assert(keys[1] == list->head->next->val);
    assert(NULL == a->prev);
    assert(keys[2] == b->next->val);
    assert(keys[0] == b->prev->val);
    assert(NULL == c->next);
    assert(keys[1] == c->prev->val);

    list_destroy (&list);
}

static void
test_list_lpush (void)
{
}

static void
test_list_at (void)
{
}

static void
test_list_destroy (void) {
    // Setup
    list_t *a = list_create ();
    list_destroy (&a);

    // a b c
    list_t *b = list_create ();
    list_rpush(b, list_node_new("a"));
    list_rpush(b, list_node_new("b"));
    list_rpush(b, list_node_new("c"));
    list_destroy (&b);

    // Assertions
    list_t *c = list_create ();
    c->free = freeProxy;
    list_rpush(c, list_node_new(list_node_new("a")));
    list_rpush(c, list_node_new(list_node_new("b")));
    list_rpush(c, list_node_new(list_node_new("c")));
    list_destroy (&c);
    assert(3 == freeProxyCalls);
}

static void
test_list_value_exist (void)
{
}

static void
test_list_remove (void)
{
}

static void
test_list_rpop (void)
{
}

static void
test_list_lpop (void)
{
}

static void
test_list_iterator_t (void)
{
    list_t *list;
    list_iterator_t *iter;
    int i;
    char *key;
    void *value;

    list = list_create ();

    for (i = 0, key = keys[0]; key != NULL; i++, key = keys[i])
    {
        list_rpush (list, key);
    }

    iter = list_iterator_create (list, LIST_HEAD);
    for (i = 0, key = keys[0]; key != NULL; i++, key = keys[i])
    {
        value = list_iterator_next (iter);
        assert (value == key);
    }

    value = list_iterator_next (iter);
    assert (value == NULL);
}

int
main(int argc, char *argv[])
{
    test (list_node_new);
    test (list_rpush);
    test (list_destroy);
    test (list_lpush);
    test (list_at);
    test (list_value_exist);
    test (list_remove);
    test (list_rpop);
    test (list_lpop);
    test (list_iterator_t);
    return 0;
}
