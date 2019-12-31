#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

/* Defined in queue.c */
struct queue;

/* Initialise the list, return NULL on error */
struct queue *queue_init(void);

/* Add item to the back of the queue, also known as "enqueue" */
int queue_push (struct queue *queue, void *item);

/* Pop item off the front of the queue, also known as "dequeue" */
void *queue_pop (struct queue *queue);

/* Free the queue from memory, apply "f(item)" to each of the items that
 * have been pushed to the list */
void queue_free(struct queue *queue, void (*f)(void*));

/* Return true if the queue contains zero items, otherwise return false */
bool queue_is_empty(struct queue *queue);

/* Return the length of the queue */
int queue_len(struct queue *);

#endif /* QUEUE_H */
