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

#include "list.h"
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

    if (list->first == NULL) {
        list->first = node;
        list->last = node;
    } else {
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
    }
    list->len += 1;
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
    free(list);
}

void *list_rm (struct list *list, void *item, int (*cmp)(void*, void*))
{
    void *ret;
    if (list->len == 0)
        return NULL;

    struct node *curr = list->first;
    while (curr != NULL) {
        if (cmp(item, curr->item) == 0) {
            ret = curr->item;
            rm_node (list, curr);
            return ret;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Simply remove the node from the linked list */
static void rm_node (struct list *list, struct node *node)
{
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
    curr = list->first;
    while (curr != NULL) {
        func(curr->item, arg);
        curr = curr->next;
    }
}
