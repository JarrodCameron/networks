/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   14/10/19 10:45               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "iter.h"
#include "list.h"
#include "synch.h"
#include "util.h"

struct node {
    struct node *next;
    struct node *prev;
    void *item;
};

struct list {
    struct node *first;
    struct node *last;
    int len;
    struct lock *lock;
};

/* Helper functions */
static struct node *init_node (void *item);
static void rm_node (struct list *list, struct node *node);

struct list *list_init (void)
{
    struct list *ret = malloc(sizeof(struct list));
    if (!ret)
        return NULL;

    *ret = (struct list) {0};

    ret->lock = lock_init();
    if (ret->lock == NULL) {
        free(ret);
        return NULL;
    }

    return ret;
}

int list_add (struct list *list, void *item)
{
    if (list == NULL)
        return -1;

    struct node *node = init_node(item);
    if (node == NULL) {
        return -1;
    }

    lock_acquire(list->lock);
    if (list->first == NULL) {
        list->first = node;
        list->last = node;
    } else {
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
    }
    list->len += 1;
    lock_release(list->lock);
    return 0;
}

void list_free (struct list *list, void (*f)(void*))
{
    if (list == NULL)
        return;

    struct node *prev = NULL;
    struct node *curr = list->first;
    while (curr != NULL) {
        prev = curr;
        curr = curr->next;
        if (f)
            f(prev->item);
        free(prev);
    }
    lock_free(list->lock);
    free(list);
}

void *list_rm (struct list *list, void *item, int (*cmp)(void*, void*))
{
    void *ret;

    lock_acquire(list->lock);
    if (list->len == 0) {
        lock_release(list->lock);
        return NULL;
    }

    struct node *curr = list->first;
    while (curr != NULL) {
        if (cmp(item, curr->item) == 0) {
            ret = curr->item;
            rm_node (list, curr);
            free(curr);
            lock_release(list->lock);
            return ret;
        }
        curr = curr->next;
    }
    lock_release(list->lock);
    return NULL;
}

void *list_get (struct list *list, int (*cmp)(void*, void*), void *arg)
{
    struct node *curr;

    lock_acquire(list->lock);
    if (list->len == 0) {
        lock_release(list->lock);
        return NULL;
    }

    curr = list->first;

    while (curr != NULL) {
        if (cmp(curr->item, arg) == 0) {
            lock_release(list->lock);
            return curr->item;
        }

        curr = curr->next;
    }
    lock_release(list->lock);
    return NULL;
}

/* Simply remove the node from the linked list */
static void rm_node (struct list *list, struct node *node)
{
    if (node == NULL)
        return;

    if (list->first == node) {
        list->first = node->next;
    } else {
        node->prev->next = node->next;
    }

    if (list->last == node) {
        list->last = node->prev;
    } else {
        node->next->prev = node->prev;
    }
    list->len -= 1;
}

/* Initialise a node and return it, NULL on error */
static struct node *init_node (void *item)
{
    struct node *ret = malloc(sizeof(struct node));
    if (!ret)
        return ret;
    ret->next = ret->prev = NULL;
    ret->item = item;
    return ret;
}

void list_traverse (struct list *list, void(*func)(void*, void*), void *arg)
{
    struct node *curr;
    lock_acquire(list->lock);
    curr = list->first;
    while (curr != NULL) {
        func(curr->item, arg);
        curr = curr->next;
    }
    lock_release(list->lock);
}

int list_len(struct list *list)
{
    int ret;
    lock_acquire(list->lock);
    ret = list->len;
    lock_release(list->lock);
    return ret;
}

void *list_pop(struct list *list)
{
    if (list == NULL)
        return NULL;

    void *ret;
    lock_acquire(list->lock);
    if (list->len == 0)
        ret = NULL;
    else {
        ret = list->first->item;
        rm_node(list, list->first);
    }
    lock_release(list->lock);

    return ret;
}

/* Return the next node after "n" */
static void *set_iter_next(UNUSED void *c, void *n)
{
    struct node *curr = n;
    return curr->next;
}

/* Return if there is a next node in the list */
static bool set_iter_has_next(UNUSED void *c, void *n)
{
    return (n != NULL);
}

/* Return the item from the node */
static void *set_iter_get(UNUSED void *c, void *n)
{
    struct node *curr = n;
    return curr->item;
}

struct iter *list_iter_init(struct list *list)
{
    assert(list != NULL);

    struct iter_funcs iter_funcs = {0};
    iter_funcs.next = set_iter_next;
    iter_funcs.has_next = set_iter_has_next;
    iter_funcs.get = set_iter_get;

    struct iter *ret = iter_init(&iter_funcs, list, list->first);
    return ret;
}

bool list_is_empty(struct list *list)
{
    bool ret;
    lock_acquire(list->lock);
    ret = (list->len == 0);
    lock_release(list->lock);
    return ret;
}
