/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   06/11/19 13:21               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdlib.h>

#include "queue.h"
#include "synch.h"

// TODO /* Pop item off the front of the queue, also known as "dequeue" */
// TODO void *queue_pop (struct queue *queue);

// TODO /* Free the queue from memory, apply "f(item)" to each of the items that
// TODO  * have been pushed to the list */
// TODO void queue_free(struct queue *queue, void (*f)(void*));


struct node {
    struct node *next;
    struct node *prev;
    void *item;
};

struct queue {
    struct node *first;
    struct node *last;
    int len;
    struct lock *lock;
};

/* Helper functions */
static struct node *init_node (void *item);

struct queue *queue_init(void)
{
    struct queue *ret = malloc(sizeof(struct queue));
    if (ret == NULL)
        return NULL;

    *ret = (struct queue) {0};

    ret->lock = lock_init();
    if (ret->lock == NULL) {
        free (ret);
        return NULL;
    }

    return ret;
}

int queue_push (struct queue *queue, void *item)
{
    if (queue == NULL)
        return -1;

    struct node *node = init_node(item);
    if (node == NULL)
        return -1;

    lock_acquire(queue->lock);
    if (queue->first == NULL) {
        queue->first = node;
        queue->last = node;
    } else {
        node->next = queue->last;
        queue->last->prev = node;
        queue->last = node;
    }
    queue->len += 1;
    lock_release(queue->lock);
    return 0;
}

/* Initialise a node and return it containing the item, return NULL on error */
static struct node *init_node (void *item)
{
    struct node *ret = malloc(sizeof(struct node));
    if (ret == NULL)
        return NULL;

    *ret = (struct node) {0};
    ret->item = item;
    return ret;
}

void *queue_pop (struct queue *queue)
{

    void *item;
    struct node *dead_node;

    if (queue == NULL)
        return NULL;

    lock_acquire(queue->lock);
    if (queue->len == 0) {
        dead_node = NULL;
    } else if (queue->len == 1) {
        dead_node = queue->first;
        queue->first = queue->last = NULL;
        queue->len = 0;
    } else {
        dead_node = queue->first;
        queue->first = queue->first->prev;
        queue->first->next = NULL;
        queue->len -= 1;
    }
    lock_release(queue->lock);

    item = dead_node->item;
    free (dead_node);
    return item;
}

void queue_free(struct queue *queue, void (*f)(void *))
{
    if (queue == NULL)
        return;

    struct node *prev = NULL;
    struct node *curr = queue->first;
    while (curr != NULL) {
        prev = curr;
        curr = curr->next;
        if (f)
           f(prev->item);
        free(prev);
    }
    lock_free(queue->lock);
    free(queue);
}

bool queue_is_empty(struct queue *q)
{
    assert (q != NULL);
    bool ret;
    lock_acquire(q->lock);
    ret = (q->len == 0);
    lock_release(q->lock);
    return ret;
}
