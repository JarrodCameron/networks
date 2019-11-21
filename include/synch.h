#ifndef SYNCH_H
#define SYNCH_H

/* This is a wrapper around the GNU pthreads library using the same functions
 * as seen in the OS course. It also provides easier to remember function
 * names */

/* Defined in synch.c */
struct lock;

/* Create a lock, currently unlocked */
struct lock *lock_init(void);

/* Acquire the lock, block until the lock is released */
void lock_acquire(struct lock *lock);

/* Release the lock, unblocked other waiting locks */
void lock_release(struct lock *lock);

/* Free the lock from memory */
void lock_free(struct lock *lock);

#endif /* SYNCH_H */
