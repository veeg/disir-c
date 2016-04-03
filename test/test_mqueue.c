
#include "mqueue.h"

struct mqueue_test_entry
{
    int a;
    struct mqueue_test_entry *next, *prev;
};

struct mqueue_test_container
{
    struct mqueue_test_entry a;
    struct mqueue_test_entry b;
    struct mqueue_test_entry c;
    struct mqueue_test_entry d;
    struct mqueue_test_entry e;
} mqueue_container;

#define MQUEUE_TEST_CONTAINER_NUMENTRIES \
    (sizeof (struct mqueue_test_container) / sizeof (struct mqueue_test_entry))

int
setup_mqueue (void **state)
{
    memset (&mqueue_container, 0, sizeof (struct mqueue_test_container));
    return (0);
}

void
test_mqueue_enqueue_untail (void **state)
{
    struct mqueue_test_entry *queue;
    struct mqueue_test_entry *entry;

    queue = NULL;
    entry = NULL;

    LOG_TEST_START

    MQ_ENQUEUE (queue, &mqueue_container.a);
    MQ_ENQUEUE (queue, &mqueue_container.b);
    MQ_ENQUEUE (queue, &mqueue_container.c);
    MQ_ENQUEUE (queue, &mqueue_container.d);
    MQ_ENQUEUE (queue, &mqueue_container.e);
    assert_ptr_equal (queue, &mqueue_container.a);

    entry = MQ_HEAD (queue);
    assert_ptr_equal (entry, queue);
    entry = MQ_TAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.e);

    entry = MQ_HEAD (queue);
    assert_ptr_equal (entry->next, &mqueue_container.b);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.c);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.d);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.e);
    entry = entry->next;
    assert_null (entry->next);

    entry = MQ_UNTAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.e);
    assert_ptr_equal (MQ_TAIL(queue), &mqueue_container.d);

    entry = MQ_UNTAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.d);
    assert_ptr_equal (MQ_TAIL(queue), &mqueue_container.c);

    entry = MQ_UNTAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.c);
    assert_ptr_equal (MQ_TAIL(queue), &mqueue_container.b);

    entry = MQ_UNTAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.b);
    assert_ptr_equal (MQ_TAIL(queue), &mqueue_container.a);

    entry = MQ_UNTAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.a);
    assert_null (queue);


    LOG_TEST_END
}

void
test_mqueue_push_pop (void **state)
{
    struct mqueue_test_entry *queue;
    struct mqueue_test_entry *entry;

    queue = NULL;
    entry = NULL;

    LOG_TEST_START

    MQ_PUSH (queue, &mqueue_container.e);
    MQ_PUSH (queue, &mqueue_container.d);
    MQ_PUSH (queue, &mqueue_container.c);
    MQ_PUSH (queue, &mqueue_container.b);
    MQ_PUSH (queue, &mqueue_container.a);

    entry = MQ_HEAD (queue);
    assert_ptr_equal (entry, queue);
    entry = MQ_TAIL (queue);
    assert_ptr_equal (entry, &mqueue_container.e);

    entry = MQ_HEAD (queue);
    assert_ptr_equal (entry->next, &mqueue_container.b);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.c);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.d);
    entry = entry->next;
    assert_ptr_equal (entry->next, &mqueue_container.e);
    entry = entry->next;
    assert_null (entry->next);

    entry = MQ_POP (queue);
    assert_ptr_equal (entry, &mqueue_container.a);
    assert_ptr_equal (queue, &mqueue_container.b);

    entry = MQ_POP (queue);
    assert_ptr_equal (entry, &mqueue_container.b);
    assert_ptr_equal (queue, &mqueue_container.c);

    entry = MQ_POP (queue);
    assert_ptr_equal (entry, &mqueue_container.c);
    assert_ptr_equal (queue, &mqueue_container.d);

    entry = MQ_POP (queue);
    assert_ptr_equal (entry, &mqueue_container.d);
    assert_ptr_equal (queue, &mqueue_container.e);

    entry = MQ_POP (queue);
    assert_ptr_equal (entry, &mqueue_container.e);
    assert_null (queue);



    LOG_TEST_END
}

const struct CMUnitTest disir_mqueue_tests[] = {
    // introduced can add
    cmocka_unit_test_setup (test_mqueue_enqueue_untail, setup_mqueue),
    cmocka_unit_test_setup (test_mqueue_push_pop, setup_mqueue),
};

