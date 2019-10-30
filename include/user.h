#ifndef USER_H
#define USER_H

#include <stdbool.h>
#include <stdint.h>

#include "list.h"
#include "config.h"

struct user;

/* Given the username and password, create and return a user. */
struct user *init_user (const char uname[MAX_UNAME], const char pword[MAX_PWORD]);

/* Free the user from memory */
void free_user (struct user *user);

/* Return strncmp() for uname equal to the user's username */
int user_uname_cmp (struct user *user, const char uname[MAX_UNAME]);

/* Return strncmp() for pword equal to the user's password */
int user_pword_cmp (struct user *user, const char pword[MAX_PWORD]);

/* Return the id of the user */
uint32_t user_getid (struct user *user);

/* Return the user with this username, NULL if doesn't exist */
struct user *user_get_by_name (struct list *users, const char uname[MAX_UNAME]);

/* This function is invoked when the user enters an invalid password too many
 * times, and is therefore blocked */
void user_set_blocked(struct user *user);

/* Return true/false if this user is blocked */
bool user_is_blocked(struct user *user);

/* Return the name of user, return value must be free()'d, NULL on error. */
char *user_get_uname(struct user *user);

/* Set's the users' logged in status to logged in. Zero is returned on success,
 * -1 on error (e.g. user already logged in) */
enum status_code user_log_on(struct user *user);

/* Return true if the user is logged on, false if the user is not logged on */
bool user_is_logged_on(struct user *user);

// For debugging, print the list of users
void user_list_dump(struct list *users);

// For debuggin, dump contents of a user
void user_dump (struct user *user);

#endif /* USER_H */
