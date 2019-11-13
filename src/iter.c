/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   12/11/19  9:55               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdlib.h>

#include "iter.h"
#include "synch.h"

struct iter {
    struct lock *lock;
    void *container;            /* The type of object (e.g. list) */
    void *curr;                 /* The object to iterate (e.g. node) */
    struct iter_funcs ifuncs;   /* Operations for nodes */
};

struct iter *iter_init(struct iter_funcs *iter_funcs, void *cont, void *first)
{
    assert(iter_funcs != NULL);
    assert(cont != NULL);

    struct iter *ret = malloc(sizeof(struct iter));
    if (ret == NULL)
        return NULL;

    ret->lock = lock_init();
    if (ret->lock == NULL) {
        free (ret);
        return NULL;
    }

    ret->container = cont;
    ret->curr = first;
    ret->ifuncs = *iter_funcs;

    return ret;
}

void iter_free(struct iter *iter)
{
    if (iter == NULL)
        return;

    struct lock *lock = iter->lock;
    lock_acquire(lock);
    free(iter);
    lock_release(lock);
    lock_free(lock);
}

void iter_next(struct iter *iter)
{
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.next != NULL);
    iter->curr = iter->ifuncs.next(iter->container, iter->curr);
    lock_release(iter->lock);
}

void iter_prev(struct iter *iter)
{
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.prev != NULL);
    iter->curr = iter->ifuncs.prev(iter->container, iter->curr);
    lock_release(iter->lock);
}

void iter_first(struct iter *iter)
{
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.first != NULL);
    iter->curr = iter->ifuncs.first(iter->container, iter->curr);
    lock_release(iter->lock);
}

void iter_last(struct iter *iter)
{
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.last != NULL);
    iter->curr = iter->ifuncs.last(iter->container, iter->curr);
    lock_release(iter->lock);
}

void *iter_get(struct iter *iter)
{
    void *ret;
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.get != NULL);
    ret = iter->ifuncs.get(iter->container, iter->curr);
    lock_release(iter->lock);
    return ret;
}

bool iter_has_next(struct iter *iter)
{
    bool ret;
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.has_next != NULL);
    ret = iter->ifuncs.has_next(iter->container, iter->curr);
    lock_release(iter->lock);
    return ret;
}

bool iter_has_prev(struct iter *iter)
{
    bool ret;
    assert(iter != NULL);
    lock_acquire(iter->lock);
    assert(iter->ifuncs.has_prev != NULL);
    ret = iter->ifuncs.has_prev(iter->container, iter->curr);
    lock_release(iter->lock);
    return ret;
}
