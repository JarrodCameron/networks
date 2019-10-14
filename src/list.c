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
struct node *init_node (void *item);

struct list *init_list (void)
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

/* Initialise a node and return it, NULL on error */
struct node *init_node (void *item)
{
    struct node *ret = malloc(sizeof(struct node));
    if (!ret)
        return ret;
    ret->next = ret->prev = NULL;
    ret->item = item;
    return ret;
}
