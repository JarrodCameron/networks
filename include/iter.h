#ifndef ITER_H
#define ITER_H

/* This is a general purpose iterator used for iterable data structures
 * without exposing the internal workings. This is similar to C++
 * iterators in the STL .
 *
 * The iter_next, iter_prev, ... functions all assert that the appropriate
 * iter functions were passed via the constructor. If an assert is raised
 * then check the "iter_funcs" input in the "iter_init" function.
 */

#include <stdbool.h>

/* Defined in iter.c */
struct iter;

/* Generic functions for the iterator */
typedef void *(*iter_func_next)(void *container, void *node);
typedef void *(*iter_func_prev)(void *container, void *node);
typedef void *(*iter_func_first)(void *container, void *node);
typedef void *(*iter_func_last)(void *container, void *node);
typedef void *(*iter_func_get)(void *container, void *node);
typedef bool (*iter_func_has_next)(void *container, void *node);
typedef bool (*iter_func_has_prev)(void *container, void *node);

struct iter_funcs {         /* Functions which all the iter to operate */
    iter_func_next next;            /* Return the next node */
    iter_func_prev prev;            /* Return the prev node */
    iter_func_first first;          /* Return the first node */
    iter_func_last last;            /* Return the last node */
    iter_func_get get;              /* Return the current item */
    iter_func_has_next has_next;    /* Return if there is a next node */
    iter_func_has_prev has_prev;    /* Return if there is a prev node */
};

/* Initialise an iterator with the respective functions for the iterator */
struct iter *iter_init
(
    struct iter_funcs *iter_funcs,
    void *container,
    void *first
);

/* Free the iterator from memroy */
void iter_free(struct iter *iter);

/* Set the iterator to the next item in the list */
void iter_next(struct iter *iter);

/* Set the iterator to the prev item in the list */
void iter_prev(struct iter *iter);

/* Set the iterator to the first item in the list */
void iter_first(struct iter *iter);

/* Set the iterator to the last item in the list */
void iter_last(struct iter *iter);

/* Return the item pointed in the list */
void *iter_get(struct iter *iter);

/* Return true if there is a next node, else return false */
bool iter_has_next(struct iter *iter);

/* Return true if there is a prev node, else return false */
bool iter_has_prev(struct iter *iter);

#endif /* ITER_H */
