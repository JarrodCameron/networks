#ifndef LIST_H
#define LIST_H

struct list;

/* Initialise the list, return NULL on error */
struct list *init_list (void);

/* Add an item to the list */
int list_add (struct list *list, void *item);

/* Free the list from memory, use `f' to free each item in the list */
void list_free (struct list *list, void (*f)(void*));

#endif /* LIST_H */
