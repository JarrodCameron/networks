#ifndef LIST_H
#define LIST_H

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

/* Iterate over all items in the list appllying func(item, arg)
 * "item" is the item passed via list_add
 * "arg" is passed to func() for each node */
void list_traverse (
    struct list *list,
    void(*func)(void *item, void *arg),
    void *arg
);

#endif /* LIST_H */
