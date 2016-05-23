#include <gtest/gtest.h>
#include <map>

// PRIVATE API
extern "C" {
#include "mqueue.h"
}

#include "test_helper.h"


struct mqueue_test_entry
{
    int value;
    struct mqueue_test_entry *next, *prev;
};

struct mqueue_test_container
{
    struct mqueue_test_entry a;
    struct mqueue_test_entry b;
    struct mqueue_test_entry c;
    struct mqueue_test_entry d;
    struct mqueue_test_entry e;
};

#define MQUEUE_TEST_CONTAINER_NUMENTRIES \
    (sizeof (struct mqueue_test_container) / sizeof (struct mqueue_test_entry))


// GLOBAL TEST VARIABLES
static struct mqueue_test_container mqueue_container;
struct mqueue_test_entry *queue;
struct mqueue_test_entry *entry;
struct mqueue_test_entry *head;
struct mqueue_test_entry *tail;

class MQueueEmptyTest : public testing::Test
{
    void SetUp()
    {
        memset (&mqueue_container, 0, sizeof (struct mqueue_test_container));
        queue = NULL;
        entry = NULL;
        head = NULL;
        tail = NULL;
    }

    void TearDown()
    {
    }
};

class MQueuePopulatedTest : public testing::Test
{
    void SetUp()
    {
        memset (&mqueue_container, 0, sizeof (struct mqueue_test_container));
        queue = NULL;
        entry = NULL;
        head = NULL;
        tail = NULL;

        mqueue_container.a.value = 10;
        mqueue_container.b.value = 10;
        mqueue_container.c.value = 5;
        mqueue_container.d.value = 10;
        mqueue_container.e.value = 9;

        MQ_ENQUEUE (queue, &mqueue_container.a);
        MQ_ENQUEUE (queue, &mqueue_container.b);
        MQ_ENQUEUE (queue, &mqueue_container.c);
        MQ_ENQUEUE (queue, &mqueue_container.d);
        MQ_ENQUEUE (queue, &mqueue_container.e);
    }

    void TearDown()
    {
    }

    public:
    void printPointerMap (void)
    {
        std::map<mqueue_test_entry *, std::string> m
            {
                { &mqueue_container.a, "A" },
                { &mqueue_container.b, "B" },
                { &mqueue_container.c, "C" },
                { &mqueue_container.d, "D" },
                { &mqueue_container.e, "E" },
            };

        head = MQ_HEAD (queue);
        std::cout << "HEAD: " << m[head] << std::endl;
        std::cout << "HEAD->next " << m[head->next] << std::endl;
        std::cout << "HEAD->prev " << m[head->prev] << std::endl;

        tail = MQ_TAIL (queue);
        std::cout << "TAIL: " << m[tail] << std::endl;
        std::cout << "TAIL->next: " << m[tail->next] << std::endl;
        std::cout << "TAIL->prev: " << m[tail->prev] << std::endl;
    }
};


TEST_F (MQueueEmptyTest, enqueue)
{
    //! Enqueue entries
    MQ_ENQUEUE (queue, &mqueue_container.a);
    MQ_ENQUEUE (queue, &mqueue_container.b);
    MQ_ENQUEUE (queue, &mqueue_container.c);
    MQ_ENQUEUE (queue, &mqueue_container.d);
    MQ_ENQUEUE (queue, &mqueue_container.e);

    ASSERT_EQ (queue, &mqueue_container.a);

    entry = queue;

    ASSERT_EQ (entry->next, &mqueue_container.b);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.c);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.d);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.e);
    entry = entry->next;
    ASSERT_TRUE (entry->next == NULL);
}

TEST_F (MQueuePopulatedTest, untail)
{
    entry = MQ_UNTAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.e);
    ASSERT_EQ (queue->prev, &mqueue_container.d);
    ASSERT_TRUE ((&mqueue_container.d)->next == NULL);

    entry = MQ_UNTAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.d);
    ASSERT_EQ (queue->prev, &mqueue_container.c);
    ASSERT_TRUE ((&mqueue_container.c)->next == NULL);

    entry = MQ_UNTAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.c);
    ASSERT_EQ (queue->prev, &mqueue_container.b);
    ASSERT_TRUE ((&mqueue_container.b)->next == NULL);

    entry = MQ_UNTAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.b);
    ASSERT_EQ (queue->prev, &mqueue_container.a);
    ASSERT_TRUE ((&mqueue_container.a)->next == NULL);

    entry = MQ_UNTAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.a);
    ASSERT_TRUE (queue == NULL);

    entry = MQ_UNTAIL (queue);
    ASSERT_TRUE (entry == NULL);
}

TEST_F (MQueueEmptyTest, push)
{
    MQ_PUSH (queue, &mqueue_container.e);
    MQ_PUSH (queue, &mqueue_container.d);
    MQ_PUSH (queue, &mqueue_container.c);
    MQ_PUSH (queue, &mqueue_container.b);
    MQ_PUSH (queue, &mqueue_container.a);

    entry = queue;

    ASSERT_EQ (entry->next, &mqueue_container.b);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.c);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.d);
    entry = entry->next;
    ASSERT_EQ (entry->next, &mqueue_container.e);
    entry = entry->next;
    ASSERT_TRUE (entry->next == NULL);
}

TEST_F (MQueuePopulatedTest, pop)
{
    entry = MQ_POP (queue);
    ASSERT_EQ (entry, &mqueue_container.a);
    ASSERT_EQ (queue, &mqueue_container.b);
    ASSERT_EQ (queue->prev, &mqueue_container.e);

    entry = MQ_POP (queue);
    ASSERT_EQ (entry, &mqueue_container.b);
    ASSERT_EQ (queue, &mqueue_container.c);
    ASSERT_EQ (queue->prev, &mqueue_container.e);

    entry = MQ_POP (queue);
    ASSERT_EQ (entry, &mqueue_container.c);
    ASSERT_EQ (queue, &mqueue_container.d);
    ASSERT_EQ (queue->prev, &mqueue_container.e);

    entry = MQ_POP (queue);
    ASSERT_EQ (entry, &mqueue_container.d);
    ASSERT_EQ (queue, &mqueue_container.e);
    ASSERT_EQ (queue->prev, &mqueue_container.e);

    entry = MQ_POP (queue);
    ASSERT_EQ (entry, &mqueue_container.e);

    ASSERT_TRUE (queue == NULL);

    entry = MQ_POP (queue);
    ASSERT_TRUE (queue == NULL);
}

TEST_F (MQueuePopulatedTest, head)
{
    entry = queue;
    ASSERT_TRUE (entry == &mqueue_container.a);

    entry = NULL;

    entry = MQ_HEAD (queue);
    ASSERT_TRUE (entry == &mqueue_container.a);
}

TEST_F (MQueuePopulatedTest, tail)
{
    entry = MQ_TAIL (queue);
    ASSERT_EQ (entry, &mqueue_container.e);
}

TEST_F (MQueuePopulatedTest, size)
{
    int count = MQ_SIZE (queue);
    ASSERT_EQ (count, 5);
}

TEST_F (MQueuePopulatedTest, size_cond)
{
    int count = MQ_SIZE_COND (queue, {entry->value == 10;});
    ASSERT_EQ (count, 3);
}

TEST_F (MQueuePopulatedTest, swap_elements_head_arg1_with_mid_entry)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.a, &mqueue_container.c);

    // New head updates its pointers
    ASSERT_EQ (queue, &mqueue_container.c);
    ASSERT_EQ (queue->next, &mqueue_container.b);
    ASSERT_EQ (queue->prev, &mqueue_container.e);
    // Old head is in the middle with updated pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.d);
    ASSERT_EQ ((&mqueue_container.a)->prev, &mqueue_container.b);

    // Node inbetween updates its pointers.
    ASSERT_EQ ((&mqueue_container.b)->next, &mqueue_container.a);
    ASSERT_EQ ((&mqueue_container.b)->prev, queue);

    // node after old head updates its pointers
    ASSERT_EQ ((&mqueue_container.d)->prev, &mqueue_container.a);
}

TEST_F (MQueuePopulatedTest, swap_elements_head_arg2_with_mid_entry)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.c, &mqueue_container.a);

    // New head updates its pointers
    ASSERT_EQ (queue, &mqueue_container.c);
    ASSERT_EQ (queue->next, &mqueue_container.b);
    ASSERT_EQ (queue->prev, &mqueue_container.e);
    // Old head is in the middle with updated pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.d);
    ASSERT_EQ ((&mqueue_container.a)->prev, &mqueue_container.b);

    // Node inbetween updates its pointers.
    ASSERT_EQ ((&mqueue_container.b)->next, &mqueue_container.a);
    ASSERT_EQ ((&mqueue_container.b)->prev, queue);

    // node after old head updates its pointers
    ASSERT_EQ ((&mqueue_container.d)->prev, &mqueue_container.a);
}

TEST_F (MQueuePopulatedTest, swap_elements_tail_arg1_with_mid_entry)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.e, &mqueue_container.b);

    // Expect
    //  A -> E -> C -> D -> B

    tail = MQ_TAIL (queue);
    // New tail updates its pointers
    ASSERT_EQ (tail, &mqueue_container.b);
    ASSERT_TRUE (tail->next == NULL);
    ASSERT_EQ (tail->prev, &mqueue_container.d);

    // Tail neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.d)->next, tail);
    ASSERT_EQ (MQ_HEAD (queue)->prev, tail);

    // Old tail is in the middle with updated pointers
    ASSERT_EQ ((&mqueue_container.e)->next, &mqueue_container.c);
    ASSERT_EQ ((&mqueue_container.e)->prev, &mqueue_container.a);

    // old tail neighbours updates their pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.e);
    ASSERT_EQ ((&mqueue_container.c)->prev, &mqueue_container.e);
}

TEST_F (MQueuePopulatedTest, swap_elements_tail_arg2_with_mid_entry)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.b, &mqueue_container.e);

    // Expect
    //  A -> E -> C -> D -> B

    tail = MQ_TAIL (queue);
    // New tail updates its pointers
    ASSERT_EQ (tail, &mqueue_container.b);
    ASSERT_TRUE (tail->next == NULL);
    ASSERT_EQ (tail->prev, &mqueue_container.d);

    // Tail neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.d)->next, tail);
    ASSERT_EQ (MQ_HEAD (queue)->prev, tail);

    // Old tail is in the middle with updated pointers
    ASSERT_EQ ((&mqueue_container.e)->next, &mqueue_container.c);
    ASSERT_EQ ((&mqueue_container.e)->prev, &mqueue_container.a);

    // old tail neighbours updates their pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.e);
    ASSERT_EQ ((&mqueue_container.c)->prev, &mqueue_container.e);
}

TEST_F (MQueuePopulatedTest, swap_elements_head_with_tail)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.a, &mqueue_container.e);

    // Expect
    //  E -> B -> C -> D -> A

    head = MQ_HEAD (queue);
    ASSERT_EQ (head, &mqueue_container.e);

    tail = MQ_TAIL (queue);
    ASSERT_EQ (tail, &mqueue_container.a);

    // Head update pointers
    ASSERT_EQ (head->next, &mqueue_container.b);
    ASSERT_EQ (head->prev, tail);

    // Head neighbour updates their pointers
    ASSERT_EQ ((&mqueue_container.b)->prev, head);

    // Tail update pointers
    ASSERT_TRUE (tail->next == NULL);
    ASSERT_EQ (tail->prev, &mqueue_container.d);

    // Tail neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.d)->next, tail);
    ASSERT_EQ (head->prev, tail);
}

TEST_F (MQueuePopulatedTest, swap_elements_tail_with_head)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.e, &mqueue_container.a);

    // Expect
    //  E -> B -> C -> D -> A

    head = MQ_HEAD (queue);
    ASSERT_EQ (head, &mqueue_container.e);

    tail = MQ_TAIL (queue);
    ASSERT_EQ (tail, &mqueue_container.a);

    // Head update pointers
    ASSERT_EQ (head->next, &mqueue_container.b);
    ASSERT_EQ (head->prev, tail);

    // Head neighbour updates their pointers
    ASSERT_EQ ((&mqueue_container.b)->prev, head);

    // Tail update pointers
    ASSERT_TRUE (tail->next == NULL);
    ASSERT_EQ (tail->prev, &mqueue_container.d);

    // Tail neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.d)->next, tail);
    ASSERT_EQ (head->prev, tail);
}

TEST_F (MQueuePopulatedTest, swap_elements_head_with_second)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.a, &mqueue_container.b);

    // Expect
    //  B -> A -> C -> D -> E

    head = MQ_HEAD (queue);
    ASSERT_EQ (head, &mqueue_container.b);

    // new head update pointers
    ASSERT_EQ (head->next, &mqueue_container.a);
    ASSERT_EQ (head->prev, &mqueue_container.e);

    // new head neighbour updates their pointers
    ASSERT_EQ ((&mqueue_container.a)->prev, head);

    // old head update pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.c);
    ASSERT_EQ ((&mqueue_container.a)->prev, &mqueue_container.b);

    // old head neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.b)->next, &mqueue_container.a);
    ASSERT_EQ ((&mqueue_container.c)->prev, &mqueue_container.a);
}

TEST_F (MQueuePopulatedTest, swap_elements_second_with_head)
{
    MQ_SWAP_ELEMENTS (queue, &mqueue_container.b, &mqueue_container.a);

    // Expect
    //  B -> A -> C -> D -> E

    head = MQ_HEAD (queue);
    ASSERT_EQ (head, &mqueue_container.b);

    // new head update pointers
    ASSERT_EQ (head->next, &mqueue_container.a);
    ASSERT_EQ (head->prev, &mqueue_container.e);

    // new head neighbour updates their pointers
    ASSERT_EQ ((&mqueue_container.a)->prev, head);

    // old head update pointers
    ASSERT_EQ ((&mqueue_container.a)->next, &mqueue_container.c);
    ASSERT_EQ ((&mqueue_container.a)->prev, &mqueue_container.b);

    // old head neighbours updates their pointer
    ASSERT_EQ ((&mqueue_container.b)->next, &mqueue_container.a);
    ASSERT_EQ ((&mqueue_container.c)->prev, &mqueue_container.a);
}

TEST_F (MQueuePopulatedTest, foreach)
{
    MQ_FOREACH (queue, {entry->value = 80;});

    ASSERT_EQ (mqueue_container.a.value, 80);
    ASSERT_EQ (mqueue_container.b.value, 80);
    ASSERT_EQ (mqueue_container.c.value, 80);
    ASSERT_EQ (mqueue_container.d.value, 80);
    ASSERT_EQ (mqueue_container.e.value, 80);
}

TEST_F (MQueuePopulatedTest, concat)
{
    struct mqueue_test_entry *queue2;

    struct mqueue_test_entry m;
    struct mqueue_test_entry n;
    struct mqueue_test_entry o;
    struct mqueue_test_entry p;

    queue2 = NULL;
    memset (&m, 1, sizeof (struct mqueue_test_entry));
    memset (&n, 1, sizeof (struct mqueue_test_entry));
    memset (&o, 1, sizeof (struct mqueue_test_entry));
    memset (&p, 1, sizeof (struct mqueue_test_entry));

    MQ_ENQUEUE (queue2, &m);
    MQ_ENQUEUE (queue2, &n);
    MQ_ENQUEUE (queue2, &o);
    MQ_ENQUEUE (queue2, &p);

    ASSERT_EQ (MQ_HEAD (queue2), &m);
    ASSERT_EQ (MQ_TAIL (queue2), &p);

    head = MQ_HEAD (queue);
    tail = MQ_TAIL (queue);
    ASSERT_EQ (tail, &mqueue_container.e);
    ASSERT_EQ (head, &mqueue_container.a);

    queue = MQ_CONCAT (queue, queue2);

    ASSERT_EQ (tail->next, &m);
    ASSERT_EQ ((&m)->prev, tail);
    ASSERT_EQ (head->prev, &p);

    head = MQ_HEAD (queue);
    tail = MQ_TAIL (queue);
    ASSERT_EQ (head, &mqueue_container.a);
    ASSERT_EQ (tail, &p);
}

TEST_F (MQueuePopulatedTest, find_containing_entry)
{
    entry = MQ_FIND (queue, {entry->value == 5;});
    ASSERT_EQ (entry, &mqueue_container.c);
}

TEST_F (MQueuePopulatedTest, find_nonexistant_entry)
{
    entry = MQ_FIND (queue, {entry->value == 989823;});
    ASSERT_TRUE (entry == NULL);
}

TEST_F (MQueuePopulatedTest, find_idx_containing_entry)
{
    entry = MQ_FIND_IDX (queue, 0);
    ASSERT_EQ (entry, &mqueue_container.a);
    entry = MQ_FIND_IDX (queue, 1);
    ASSERT_EQ (entry, &mqueue_container.b);
    entry = MQ_FIND_IDX (queue, 2);
    ASSERT_EQ (entry, &mqueue_container.c);
    entry = MQ_FIND_IDX (queue, 3);
    ASSERT_EQ (entry, &mqueue_container.d);
    entry = MQ_FIND_IDX (queue, 4);
    ASSERT_EQ (entry, &mqueue_container.e);

}

TEST_F (MQueuePopulatedTest, find_idx_nonexistant_entry)
{
    entry = MQ_FIND_IDX (queue, 98);
    ASSERT_TRUE (entry == NULL);
    entry = MQ_FIND_IDX (queue, -1);
    ASSERT_TRUE (entry == NULL);
    entry = MQ_FIND_IDX (queue, 5);
    ASSERT_TRUE (entry == NULL);
}


TEST_F (MQueuePopulatedTest, contains_existing_entry)
{
    int pos;

    pos = MQ_CONTAINS (queue, &mqueue_container.a);
    ASSERT_EQ (pos, 1);
    pos = MQ_CONTAINS (queue, &mqueue_container.b);
    ASSERT_EQ (pos, 2);
    pos = MQ_CONTAINS (queue, &mqueue_container.c);
    ASSERT_EQ (pos, 3);
    pos = MQ_CONTAINS (queue, &mqueue_container.d);
    ASSERT_EQ (pos, 4);
    pos = MQ_CONTAINS (queue, &mqueue_container.e);
    ASSERT_EQ (pos, 5);

}

TEST_F (MQueuePopulatedTest, contains_nonexisting_entry)
{
    int pos;
    struct mqueue_test_entry m;

    pos = MQ_CONTAINS (queue, &m);
    ASSERT_EQ (pos, 0);

    pos = MQ_CONTAINS (queue, NULL);
    ASSERT_EQ (pos, 0);

}
