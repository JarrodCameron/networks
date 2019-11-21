/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   27/10/19  9:32               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <pthread.h>

#include "synch.h"

struct lock {
    pthread_mutex_t *mutex;
};

struct lock *lock_init(void)
{
    struct lock *ret = malloc(sizeof(struct lock));
    if (ret == NULL)
        return NULL;

    ret->mutex = malloc(sizeof(pthread_mutex_t));
    if (ret->mutex == NULL) {
        free (ret);
        return NULL;
    }

    if (pthread_mutex_init(ret->mutex, NULL) != 0) {
        free (ret->mutex);
        free (ret);
        return NULL;
    }

    return ret;
}

void lock_acquire(struct lock *lock)
{
    assert(lock);
    pthread_mutex_lock(lock->mutex);
}

void lock_release(struct lock *lock)
{
    assert(lock);
    pthread_mutex_unlock(lock->mutex);
}

void lock_free(struct lock *lock)
{
    assert(lock);

    // Just like the Joker, I don't really have a plan.
    while(pthread_mutex_destroy(lock->mutex) == EBUSY);
}
