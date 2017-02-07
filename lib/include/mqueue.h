#ifndef _LIBDISIR_PRIVATE_MQUEUE_H
#define _LIBDISIR_PRIVATE_MQUEUE_H

//
// These macros work on any structure which has a `next' and  a `prev'
// field.
//
// All arguments `q' and `x' must be pointers to structures.
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
// Macro MQ_HEAD (q)
//
//! \brief Return first element of `q`
//!
//! Does not modify the `q` structure nor its members.
//!
//! \return typeof (q) which is the first element of `q`
//! \return NULL if queue is empty.
//!
#define MQ_HEAD(q)        (q)


//
// Macro MQ_TAIL (q)
//
//! \brief Return last element of `q`.
//!
//! Does not modify the `q` structure nor its members.
//!
//! \return typeof (q) which is the last element of `q`.
//! \return NULL if queue is empty.
//!
#define MQ_TAIL(q)        ((q) ? (q)->prev : NULL)


//
// Macro MQ_ENQUEUE (q, x)
//
//! \brief Insert element `x` into tail of `q`.
//!
//! \return void
//!
#define MQ_ENQUEUE(q, x)                                                \
    {                                                                   \
        (x)->next = NULL;                                               \
                                                                        \
        if ((q) == NULL) {                                              \
            (q) = (x)->prev = (x);                                      \
        } else {                                                        \
            (q)->prev->next = (x);                                      \
            (x)->prev = (q)->prev;                                      \
            (q)->prev = (x);                                            \
        }                                                               \
    }


//
// Macro DEQUEUE (q)
//
//! \brief Remove head element from `q' and return it.
//!
//! \return typeof (q)
//! \return NULL if `q` is empty.
//!
#define MQ_DEQUEUE(q)                                                   \
    ({                                                                  \
        typeof(q) _ret;                                                 \
                                                                        \
        _ret = (q);                                                     \
        if (_ret != NULL) {                                             \
            (q) = (q)->next;                                            \
            if ((q) != NULL)                                            \
                (q)->prev = _ret->prev;                                 \
            else                                                        \
                (q) = NULL;                                             \
        }                                                               \
                                                                        \
        _ret;                                                           \
    })


//
// Macro MQ_PUSH (q, x)
//
//! \brief Insert element `x` into head of `q`.
//!
//! \return void
//!
#define MQ_PUSH(q, x)                                                   \
    {                                                                   \
        (x)->next = (q);                                                \
                                                                        \
        if ((q) == NULL) {                                              \
            (q) = (x)->prev = (x);                                      \
        } else {                                                        \
            (x)->prev = (q)->prev;                                      \
            (q)->prev = (x);                                            \
            (q) = (x);                                                  \
        }                                                               \
    }


//
// Macro MQ_POP (q)
//
//! \brief Remove head element from `q` and return it.
//!
//! Alias for MQ_DEQUEUE
//!
//! \return typeof (q)
//! \return NULL if `q` is empty.
//!
#define MQ_POP(q)          MQ_DEQUEUE(q)


//
// Macro UNTAIL (q)
//
//! \brief Remove the last element of `q` and return it.
//!
//! If queue is empty, return NULL.
//!
//! \return typeof (q)
//! \return NULL if `q` is empty.
//!
#define MQ_UNTAIL(q)                                                    \
    ({                                                                  \
        typeof(q) _ret;                                                 \
                                                                        \
        if ((q) == NULL) {                                              \
            _ret = NULL;                                                \
        } else {                                                        \
            _ret = (q)->prev;                                           \
            if ((q)->next == NULL) {                                    \
                (q) = NULL;                                             \
            } else {                                                    \
                (q)->prev = _ret->prev;                                 \
                _ret->prev->next = NULL;                                \
            }                                                           \
        }                                                               \
                                                                        \
        _ret;                                                           \
    })


//
// Macro MQ_PUT (q, x, n)
//
//! \brief Put element `x` into nth position of `q`.
//!
//! Inserted at tail if `q` does not contain `n` elements.
//!
//! \return void
//!
#define MQ_PUT(q, x, n)                                                 \
    {                                                                   \
        typeof(q) _tmp = (q), _prev = NULL;                             \
        int _cnt = (n);                                                 \
                                                                        \
        while (_tmp != NULL && _cnt-- > 0) {                            \
            _prev = _tmp;                                               \
            _tmp = _tmp->next;                                          \
        }                                                               \
                                                                        \
        (x)->next = _tmp;                                               \
        if (_prev) {                                                    \
            _prev->next = (x);                                          \
            (x)->prev = _prev;                                          \
            if (_tmp)                                                   \
                _tmp->prev = (x);                                       \
        } else if (_tmp) {                                              \
            (q) = (x);                                                  \
            (x)->prev = _tmp->prev;                                     \
            _tmp->prev = (x);                                           \
        } else {                                                        \
            (q) = (x)->prev = (x);                                      \
        }                                                               \
    }



//
// Macro MQ_REMOVE (q, x)
//
//! \brief Remove element `x' from `q'.
//!
//! WARNING: If `q' does not contain `x', the result is undefined.
//!     Use MQ_REMOVE_SAFE to guard against element not present.
//! NOTE: the prev and next pointers of x are NOT touched
//!
//! \return void
//!
#define MQ_REMOVE(q, x)                                                 \
    {                                                                   \
        if ((x) != (q)) {                                               \
            (x)->prev->next = (x)->next;                                \
            if ((x)->next != NULL) {                                    \
                (x)->next->prev = (x)->prev;                            \
            } else {                                                    \
                (q)->prev = (x)->prev;                                  \
            }                                                           \
        } else {                                                        \
            (q) = (x)->next;                                            \
            if ((q))                                                    \
                (q)->prev = (x)->prev;                                  \
        }                                                               \
    }

//
// Macro MQ_REMOVE_CONDITIONAL (q, expr)
//
//! \brief Find and remove first entry in `q' whose `expr' evaluates to true
//!
//! The symbol `entry' in expr will evaluate to
//! the entry which is being examined.
//! The expression:
//! \code
//!       entry->value == 1 || !strcmp(entry->name, "foo")
//! \endcode
//!
//! will for instance search for the first entry in queue whose
//! value field equals to 1, or whose name field equals to "foo".
//!
//! \return typeof (q) which is the matching entry removed from `q`.
//! \return NULL if no matching entry was found nor removed.
//!
#define MQ_REMOVE_CONDITIONAL(q, expr)                                  \
    ({                                                                  \
        typeof(q) entry = (q);                                          \
                                                                        \
        while (entry != NULL) {                                         \
            if ((expr)) {                                               \
                MQ_REMOVE(q, entry);                                    \
                break;                                                  \
            }                                                           \
            entry = entry->next;                                        \
        }                                                               \
                                                                        \
        entry;                                                          \
    })



//
// Macro MQ_REMOVE_SAFE (q, x)
//
//! \brief Find element `x' in queue and remove it.
//!
//! NOTE: the prev and next pointers of `x` are NOT touched.
//!
//! \return int 0 if element `x` was not in `q` and thus not removed.
//! \return int 1 if element `x` was removed from `q`.
//!
#define MQ_REMOVE_SAFE(q, x)                                            \
    ({                                                                  \
        typeof(q) _tmp = q;                                             \
        int _ret = 0;                                                   \
                                                                        \
        while (_tmp != NULL) {                                          \
            if (_tmp == (x))                                            \
                break;                                                  \
            _tmp = _tmp->next;                                          \
        }                                                               \
                                                                        \
        if (_tmp != NULL) {                                             \
            _ret = 1;                                                   \
            MQ_REMOVE(q, x);                                            \
        }                                                               \
                                                                        \
        _ret;                                                           \
    })


//
// Macro MQ_CONTAINS (q, x)
//
//! \brief Check if `x` is contained in `q`.
//!
//! \return int 0 if `x` is not conaed in `q`.
//! \return int position of `x` in `q` if it is contained within.
//!
#define MQ_CONTAINS(q, x)                                               \
    ({                                                                  \
        typeof(q) _tmp = (q);                                           \
        int _cnt = 0;                                                   \
                                                                        \
        while (_tmp != NULL) {                                          \
            _cnt++;                                                     \
            if (_tmp == (x))                                            \
                break;                                                  \
            _tmp = _tmp->next;                                          \
        }                                                               \
                                                                        \
        _tmp == NULL ? 0 : _cnt;                                        \
    })


//
// Macro MQ_FIND (q, expr)
//
//! \breif Find and return first entry in `q' whose `expr' evaluates to true
//!
//! The symbol `entry' in expr will evaluate to the
//! entry which is being examined.  The expression:
//!
//! \begincode
//!       entry->value == 1 || !strcmp(entry->name, "foo")
//! \endcode
//!
//! will for instance search for the first entry in queue whose
//! value field equals to 1, or whose name field equeals to "foo".
//!
//! \return typeof (q) whose first entry match `expr`.
//! \return NULL if no entry matched `expr`.
//!
#define MQ_FIND(q, expr)                                                \
    ({                                                                  \
        typeof(q) entry = (q);                                          \
                                                                        \
        while (entry != NULL) {                                         \
            if ((expr))                                                 \
                break;                                                  \
            entry = entry->next;                                        \
        }                                                               \
                                                                        \
        entry;                                                          \
    })


//
// Macro MQ_FIND_IDX (q, idx)
//
//! \brief Find element at index `idx` in `q`.
//!
//! \return typeof (q) at index `q` if  idx < MQ_SIZE (q)
//! \return NULL if `idx >= MQ_SIZE (q)
//!
#define MQ_FIND_IDX(q, idx)                                             \
    ({                                                                  \
        typeof(q) _tmp = (q);                                           \
        int _cnt = 0;                                                   \
                                                                        \
        while (_tmp != NULL) {                                          \
            if (_cnt == (idx))                                          \
                break;                                                  \
            _cnt++;                                                     \
            _tmp = _tmp->next;                                          \
        }                                                               \
                                                                        \
        _tmp;                                                           \
    })


//
// Macro MQ_ENQUEUE_CONDITIONAL(q, x, expr)
//
//! \brief Enqueue element x in q at the position where `expr` evaluates to true
//!
//! \return void
//!
#define MQ_ENQUEUE_CONDITIONAL(q, x, expr)                              \
    {                                                                   \
        typeof(q) entry = (q);                                          \
        typeof(q) prev;                                                 \
                                                                        \
        if (entry == NULL || (expr)) {                                  \
            MQ_PUSH(q, x);                                              \
        } else {                                                        \
            prev = entry;                                               \
            entry = entry->next;                                        \
            while (entry && !(expr)) {                                  \
                prev = entry;                                           \
                entry = entry->next;                                    \
            }                                                           \
            prev->next = x;                                             \
            x->prev = prev;                                             \
            x->next = entry;                                            \
            if (entry != NULL) {                                        \
                entry->prev = x;                                        \
            } else {                                                    \
                (q)->prev = x;                                          \
            }                                                           \
        }                                                               \
    }


//
// Macro MQ_CONCAT (q1, q2)
//
//! \brief Concatenate `q1' and `q2'.
//!
//! NOTE: `q1` and `q2` must be of same pointer type.
//! Return the concatenated queue.
//! If `q1` is NULL, `q2` is the returned pointer.
//! If both are NULL, NULL is returned.
//! NOTE !: q2 is NOT zeroed !!
//!
//! \return NULL if `q1` and `q2` are both NULL.
//! \return typeof (q1) whose value is `q1` if not NULL
//! \return typeof (q2) whose value is `q2` if `q1 is NULL and `q2` is not NULL
//!
#define MQ_CONCAT(q1, q2)                                               \
    ({                                                                  \
        if ((q1) == NULL) {                                             \
            (q1) = (q2);                                                \
        } else if ((q2)) {                                              \
            typeof(q1) _last = (q1)->prev;                              \
            _last->next = (q2);                                         \
            (q1)->prev = (q2)->prev;                                    \
            (q2)->prev = _last;                                         \
        }                                                               \
                                                                        \
        (q1);                                                           \
    })


//
// Macro MQ_SIZE (q)
//
//! \breif Count the number of elements in `q'.
//!
//! \return int number of elmeents in `q`.
//!
#define MQ_SIZE(q)                                                      \
    ({                                                                  \
        typeof(q) _tmp = (q);                                           \
        int _cnt = 0;                                                   \
                                                                        \
        while (_tmp != NULL) {                                          \
            _tmp = _tmp->next;                                          \
            _cnt++;                                                     \
        }                                                               \
                                                                        \
        _cnt;                                                           \
    })




//
// Macro MQ_SIZE_COND (q, cond)
//
//! \brief Count the number of elements in `q' given the condition
//!
//! \return int number of elmeents in `q` matching `cond`.
//!
#define MQ_SIZE_COND(q, expr)                                           \
    ({                                                                  \
        typeof(q) entry = (q);                                          \
        int _cnt = 0;                                                   \
        while (entry != NULL) {                                         \
            if ((expr))                                                 \
                _cnt++;                                                 \
            entry = entry->next;                                        \
        }                                                               \
        _cnt;                                                           \
    })

//
// Macro MQ_SWAP_ELEMENTS(a, b)
//
//! \brief Swap elements `a` and `b` in `q`.
//!
//! WARNING: If a and b are not in q the result is undefined.
//! May alter q.
//!
//! \return void
//!
#define MQ_SWAP_ELEMENTS(q, a, b)                                       \
    {                                                                   \
        typeof(q) __tmp1, __tmp2;                                       \
                                                                        \
        __tmp1 = (a)->next;                                             \
        __tmp2 = (a)->prev;                                             \
                                                                        \
        /* Do nothing if A and B are the same */                        \
        if ((a) == (b)) { }                                             \
        /* IF a is just preceeding B, or B is HEAD and A is TAIL */     \
        else if (((a)->next == (b)) || ((b)->prev == (a))) {            \
            if ((a)->prev->next != NULL) (a)->prev->next = (b);         \
            if ((b)->next != NULL)       (b)->next->prev = (a);         \
            (a)->next = (b)->next;                                      \
            (a)->prev = (b);                                            \
            if (__tmp1 == NULL) (b)->next = NULL;                       \
            else (b)->next = (a);                                       \
            (b)->prev = __tmp2;                                         \
        /* IF B is just preceeding A, or A is HEAD and B is TAIL */     \
        } else if (((b)->next == (a)) || ((a)->prev == (b))) {          \
            if ((b)->prev->next != NULL) (b)->prev->next = (a);         \
            if ((a)->next != NULL)       (a)->next->prev = (b);         \
            if ((b)->next == NULL) (a)->next = NULL;                    \
            else (a)->next = (b);                                       \
            (a)->prev = (b)->prev;                                      \
            (b)->next = __tmp1;                                         \
            (b)->prev = (a);                                            \
        } else {                                                        \
            /* Moving tail - update head->prev */                       \
            if ((b)->next == NULL)       (q)->prev = (a);               \
            if ((a)->next == NULL)       (q)->prev = (b);               \
            if ((a)->prev->next != NULL) (a)->prev->next = (b);         \
            if ((a)->next != NULL)       (a)->next->prev = (b);         \
            if ((b)->prev->next != NULL) (b)->prev->next = (a);         \
            if ((b)->next != NULL)       (b)->next->prev = (a);         \
            (a)->next = (b)->next;                                      \
            (a)->prev = (b)->prev;                                      \
            (b)->next = __tmp1;                                         \
            (b)->prev = __tmp2;                                         \
        }                                                               \
        /* SWAP Q if A or B was HEAD */                                 \
        if ((q) == (a)) (q) = (b);                                      \
        else if ((q) == (b)) (q) = (a);                                 \
    }

//
// Macro MQ_FOREACH(q)
//
//! \breif Execute `expr` on each element in `q`
//!
//! \return void
//!
#define MQ_FOREACH(q, expr)                                             \
    ({                                                                  \
        typeof(q) entry = (q);                                          \
                                                                        \
     while (entry != NULL) {                                            \
            (expr);                                                     \
            entry = entry->next;                                        \
        }                                                               \
                                                                        \
        entry;                                                          \
    })


#endif // _LIBDISIR_PRIVATE_MQUEUE_H

