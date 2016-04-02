#ifndef _LIBDISIR_MQUEUE_H
#define _LIBDISIR_MQUEUE_H

//
// These macros work on any structure which has a `next' and  a `prev'
// field.
//
// All arguments `q' and `x' should be pointers to structures.
//
// The elements are linked into a double linked list where the prev-
// pointer of the first element points to the last element in the
// list.  The next-pointer of the last element will point to NULL
// (making list traversal very easy).  A list containing three
// elements; (A, B, C) will as such be linked the following way:
//
//                  next       next       next
//    `q' ---> |===|----> |===|----> |===|----> NULL
//             | A |      | B |      | C |
//       ,-----|===| <----|===| <----|===| <-.
//       | prev       prev       prev        |
//       |                                   |
//       `-----------------------------------'
//
// It should be noted that since we are using macros, the programmer
// must be aware of the usual macro artifacts.  That is, specifying
// for instance ``some_queue[i++]'' as a macro argument is not a very
// good idea.
//


//
// Macro QHEAD (q)
//
//    Return the first element of `q', but do not remove it.  Return
//    NULL if queue is empty.
//
#define MQ_HEAD(q)        (q)


//
// Macro QTAIL (q)
//
//    Return the last element of `q', but do not remove it.  Return
//    NULL if queue is empty.
//
#define MQ_TAIL(q)        ((q) ? (q)->prev : NULL)


//
// Macro MQ_ENQUEUE (q, x)
//
//    Insert element `x' into tail of `q'.
//
#define MQ_ENQUEUE(q, x)                                            \
    ({                                                              \
        (x)->next = NULL;                                           \
                                                                    \
        if ((q) == NULL) {                                          \
            (q) = (x)->prev = (x);                                  \
        } else {                                                    \
            (q)->prev->next = (x);                                  \
            (x)->prev = (q)->prev;                                  \
            (q)->prev = (x);                                        \
        }                                                           \
    })


//
// Macro DEQUEUE (q)
//
//    Remove head element from `q' and return it.
//
#define MQ_DEQUEUE(q)                                               \
    ({                                                              \
        typeof(q) _ret;                                             \
                                                                    \
        _ret = (q);                                                 \
        if (_ret != NULL) {                                         \
            (q) = (q)->next;                                        \
            if ((q) != NULL)                                        \
                (q)->prev = _ret->prev;                             \
            else                                                    \
                (q) = NULL;                                         \
        }                                                           \
                                                                    \
        _ret;                                                       \
    })


//
// Macro PUSH (q, x)
//
//    Insert element `x' into head of `q'.
//
#define MQ_PUSH(q, x)                                               \
    ({                                                              \
        (x)->next = (q);                                            \
                                                                    \
        if ((q) == NULL) {                                          \
            (q) = (x)->prev = (x);                                  \
        } else {                                                    \
            (x)->prev = (q)->prev;                                  \
            (q)->prev = (x);                                        \
            (q) = (x);                                              \
        }                                                           \
    })


//
// Macro POP (q)
//
//    Remove head element from `q' and return it.
//    Alias for MQ_DEQUEUE
//
#define MQ_POP(q)          MQ_DEQUEUE(q)


//
// Macro UNTAIL (q)
//
//    Remove the last element of `q' and return it.  If queue is
//    empty, return NULL.
//
#define MQ_UNTAIL(q)                                                \
    ({                                                              \
        typeof(q) _ret;                                             \
                                                                    \
        if ((q) == NULL) {                                          \
            _ret = NULL;                                            \
        } else {                                                    \
            _ret = (q)->prev;                                       \
            if ((q)->next == NULL) {                                \
                (q) = NULL;                                         \
            } else {                                                \
                (q)->prev = _ret->prev;                             \
                _ret->prev->next = NULL;                            \
            }                                                       \
        }                                                           \
                                                                    \
        _ret;                                                       \
    })


//
// Macro QPUT (q, x, n)
//
//    Put element `x' into nth position of `q', or at the tail if `q'
//    does not contain `n' elements.
//
#define MQ_PUT(q, x, n)                                             \
    ({                                                              \
        typeof(q) _tmp = (q), _prev = NULL;                         \
        int _cnt = (n);                                             \
                                                                    \
        while (_tmp != NULL && _cnt-- > 0) {                        \
            _prev = _tmp;                                           \
            _tmp = _tmp->next;                                      \
        }                                                           \
                                                                    \
        (x)->next = _tmp;                                           \
        if (_prev) {                                                \
            _prev->next = (x);                                      \
            (x)->prev = _prev;                                      \
            if (_tmp)                                               \
                _tmp->prev = (x);                                   \
        } else if (_tmp) {                                          \
            (q) = (x);                                              \
            (x)->prev = _tmp->prev;                                 \
            _tmp->prev = (x);                                       \
        } else {                                                    \
            (q) = (x)->prev = (x);                                  \
        }                                                           \
    })



//
// Macro QREMOVE (q, x)
//
//    Remove element `x' from `q'.  If `q' does not contain `x', the
//    result is undefined.
//    NOTE: the prev and next pointers of x are NOT touched
//
#define MQ_REMOVE(q, x)                                             \
    ({                                                              \
        if ((x) != (q)) {                                           \
            (x)->prev->next = (x)->next;                            \
            if ((x)->next != NULL) {                                \
                (x)->next->prev = (x)->prev;                        \
            } else {                                                \
                (q)->prev = (x)->prev;                              \
            }                                                       \
        } else {                                                    \
            (q) = (x)->next;                                        \
            if ((q))                                                \
                (q)->prev = (x)->prev;                              \
        }                                                           \
    })

//
// Macro QREMOVE_CONDITIONAL (q, expr)
//
//    Find first entry in `q' whose `expr' evaluates to true, and
//    remove it.  The symbol `entry' in expr will evaluate to the
//    entry which is being examined.  The expression:
//
//       entry->value == 1 || !strcmp(entry->name, "foo")
//
//    will for instance search for the first entry in queue whose
//    value field equals to 1, or whose name field equals to "foo".
//
#define MQ_REMOVE_CONDITIONAL(q, expr)                              \
    ({                                                              \
        typeof(q) entry = (q);                                      \
                                                                    \
        while (entry != NULL) {                                     \
            if ((expr)) {                                           \
                MQ_REMOVE(q, entry);                                \
                break;                                              \
            }                                                       \
            entry = entry->next;                                    \
        }                                                           \
                                                                    \
        entry;                                                      \
    })



//
// Macro QREMOVE_SAFE (q, x)
//
//    Find element `x' in queue and remove it.  Return true if element
//    was deleted, false otherwise.
//
#define MQ_REMOVE_SAFE(q, x)                                        \
    ({                                                              \
        typeof(q) _tmp = q;                                         \
        int _ret = 0;                                               \
                                                                    \
        while (_tmp != NULL) {                                      \
            if (_tmp == (x))                                        \
                break;                                              \
            _tmp = _tmp->next;                                      \
        }                                                           \
                                                                    \
        if (_tmp != NULL) {                                         \
            _ret = 1;                                               \
            MQ_REMOVE(q, x);                                        \
        }                                                           \
                                                                    \
        _ret;                                                       \
    })


//
// Macro QCONTAINS (q, x)
//
//    Return true if `q' contains the element `x'.   Return false
//    otherwise.  The value returned is actually the position of `x'
//    in `q'.
//
#define MQ_CONTAINS(q, x)                                           \
    ({                                                              \
        typeof(q) _tmp = (q);                                       \
        int _cnt = 0;                                               \
                                                                    \
        while (_tmp != NULL) {                                      \
            _cnt++;                                                 \
            if (_tmp == (x))                                        \
                break;                                              \
            _tmp = _tmp->next;                                      \
        }                                                           \
                                                                    \
        _tmp == NULL ? 0 : _cnt;                                    \
    })


//
// Macro MQ_FIND (q, expr)
//
//    Find first entry in `q' whose `expr' evaluates to true, and
//    return it.  The symbol `entry' in expr will evaluate to the
//    entry which is being examined.  The expression:
//
//       entry->value == 1 || !strcmp(entry->name, "foo")
//
//    will for instance search for the first entry in queue whose
//    value field equals to 1, or whose name field equeals to "foo".
//
#define MQ_FIND(q, expr)                                            \
    ({                                                              \
        typeof(q) entry = (q);                                      \
                                                                    \
        while (entry != NULL) {                                     \
            if ((expr))                                             \
                break;                                              \
            entry = entry->next;                                    \
        }                                                           \
                                                                    \
        entry;                                                      \
    })


//
// Macro MQ_FIND_IDX (q, idx)
//
//    Return element at position idx.   Return NULL if no such element
//
#define MQ_FIND_IDX(q, idx)                                         \
    ({                                                              \
        typeof(q) _tmp = (q);                                       \
        int _cnt = 0;                                               \
                                                                    \
        while (_tmp != NULL) {                                      \
            if (_cnt == (idx))                                      \
                break;                                              \
            _cnt++;                                                 \
            _tmp = _tmp->next;                                      \
        }                                                           \
                                                                    \
        _tmp;                                                       \
    })


//
// Macro MQ_ENQUEUE_CONDITIONAL(q, x, expr)
//
//   Enqueue element x in q at the position where expr
//   evaluates to true
//
#define MQ_ENQUEUE_CONDITIONAL(q, x, expr)                          \
    ({                                                              \
        typeof(q) entry = (q);                                      \
        typeof(q) prev;                                             \
                                                                    \
        if (entry == NULL || (expr)) {                              \
            MQ_PUSH(q, x);                                          \
        } else {                                                    \
            prev = entry;                                           \
            entry = entry->next;                                    \
            while (entry && !(expr)) {                              \
                prev = entry;                                       \
                entry = entry->next;                                \
            }                                                       \
            prev->next = x;                                         \
            x->prev = prev;                                         \
            x->next = entry;                                        \
            if (entry != NULL) {                                    \
                entry->prev = x;                                    \
            } else {                                                \
                (q)->prev = x;                                      \
            }                                                       \
        }                                                           \
    })


//
// Macro MQ_CONCAT (q1, q2)
//
//    Concatenate `q1' and `q2'.  Return the concatenated queue.
//    NOTE !: q2 is NOT zeroed !!
//
#define MQ_CONCAT(q1, q2)                                           \
    ({                                                              \
        if ((q1) == NULL) {                                         \
            (q1) = (q2);                                            \
        } else if ((q2)) {                                          \
            typeof(q1) _last = (q1)->prev;                          \
            _last->next = (q2);                                     \
            (q1)->prev = (q2)->prev;                                \
            (q2)->prev = _last;                                     \
        }                                                           \
                                                                    \
        (q1);                                                       \
    })


//
// Macro MQ_SIZE (q)
//
//    Count the number of elements in `q'.
//
#define MQ_SIZE(q)                                                  \
    ({                                                              \
        typeof(q) _tmp = (q);                                       \
        int _cnt = 0;                                               \
                                                                    \
        while (_tmp != NULL) {                                      \
            _tmp = _tmp->next;                                      \
            _cnt++;                                                 \
        }                                                           \
                                                                    \
        _cnt;                                                       \
    })




//
// Macro MQ_SIZE_COND (q, cond)
//
//    Count the number of elements in `q' given the condition
//
#define MQ_SIZE_COND(q, expr)                                       \
    ({                                                              \
        typeof(q) entry = (q);                                      \
        int _cnt = 0;                                               \
        while (entry != NULL) {                                     \
            if ((expr))                                             \
                _cnt++;                                             \
            entry = entry->next;                                    \
        }                                                           \
        _cnt;                                                       \
    })

//
// Macro MQ_SWAP_ELEMENTS(a, b)
//
//    Swap elements a and b in q.  If a and b are not in q,
//    or if a and b are the same, the result is undefined.  May alter q.

#define MQ_SWAP_ELEMENETS(q, a, b)                                  \
({                                                                  \
    typeof(q) __tmp1, __tmp2;                                       \
                                                                    \
    __tmp1 = (a)->next;                                             \
    __tmp2 = (a)->prev;                                             \
                                                                    \
    if (((a)->next == (b)) || ((b)->prev == (a))) {                 \
        if ((a)->prev->next != NULL) (a)->prev->next = (b);         \
        if ((b)->next != NULL)       (b)->next->prev = (a);         \
        (a)->next = (b)->next;                                      \
        (a)->prev = (b);                                            \
        if (__tmp1 == NULL) (b)->next = NULL;                       \
        else (b)->next = (a);                                       \
        (b)->prev = __tmp2;                                         \
    } else if (((b)->next == (a)) || ((a)->prev == (b))) {          \
        if ((b)->prev->next != NULL) (b)->prev->next = (a);         \
        if ((a)->next != NULL)       (a)->next->prev = (b);         \
        if ((b)->next == NULL) (a)->next = NULL;                    \
        else (a)->next = (b);                                       \
        (a)->prev = (b)->prev;                                      \
        (b)->next = __tmp1;                                         \
        (b)->prev = (a);                                            \
    } else {                                                        \
        if ((a)->prev->next != NULL) (a)->prev->next = (b);         \
        if ((a)->next != NULL)       (a)->next->prev = (b);         \
        if ((b)->prev->next != NULL) (b)->prev->next = (a);         \
        if ((b)->next != NULL)       (b)->next->prev = (a);         \
        (a)->next = (b)->next;                                      \
        (a)->prev = (b)->prev;                                      \
        (b)->next = __tmp1;                                         \
        (b)->prev = __tmp2;                                         \
    }                                                               \
    if ((q) == (a)) (q) = (b);                                      \
    else if ((q) == (b)) (q) = (a);                                 \
})

//
// Macro MQ_FOREACH(q)
//
//     Execute 'expr' on each element in 'q'
//
#define MQ_FOREACH(q, expr)                                         \
    ({                                                              \
        typeof(q) entry = (q);                                      \
                                                                    \
     while (entry != NULL) {                                        \
            (expr);                                                 \
            entry = entry->next;                                    \
        }                                                           \
                                                                    \
        entry;                                                      \
        })



#endif // _LIBDISIR_MQUEUE_H

