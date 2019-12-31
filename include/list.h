#ifndef LIST_H
#define LIST_H

#include "iter.h"

/* Defined in list.c */
struct list;

/* Initialise the list, return NULL on error */
struct list *list_init (void);

/* Add an item to the list */
int list_add (struct list *list, void *item);

/* Free the list from memory, use `f' to free each item in the list */
void list_free (struct list *list, void (*f)(void*));

/* Remove an item from the list.
 * cmp takes two items and compares them, returns the difference,
 *   return 0 if equal
 * `item' is the item to be removed (was passed by list_add)
 * The item is returned, NULL if no item is returned.
 */
void *list_rm (struct list *list, void *item, int (*cmp)(void*, void*));

/* Iterate over all items in the list applying func(item, arg)
 * "item" is the item passed via list_add
 * "arg" is passed to func() for each node */
void list_traverse (
    struct list *list,
    void(*func)(void *item, void *arg),
    void *arg
);

/* Return the item in the list where "cmp" returns 0. The "arg" is passed into
 * "cmp" and the "item" is the item from "list_add". */
void *list_get (
    struct list *list,
    int(*cmp)(void *item, void *arg),
    void *arg
);

/* Return the number of nodes in the list */
int list_len(struct list *list);

/* Pop a random item of the list, most likely the first or the last,
 * which item that is popped of is undefined */
void *list_pop(struct list *list);

/* Return true if the list is empty, otherwise return false */
bool list_is_empty(struct list *list);

/* Create an iterator to traverse the list */
struct iter *list_iter_init(struct list *list);

#endif /* LIST_H */
