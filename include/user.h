#ifndef USER_H
#define USER_H

#include "list.h"
#include "config.h"

/* Defined in user.c */
struct user;

/* Given the username and password, create and return a user. */
struct user *user_init (const char uname[MAX_UNAME], const char pword[MAX_PWORD]);

/* Free the user from memory */
void user_free (struct user *user);

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

/* Log of the user. If the user is already logged off then do nothing */
void user_log_off(struct user *user);

/* Return true if the user is logged on, false if the user is not logged on */
bool user_is_logged_on(struct user *user);

/* Return a list of users for the whoelse command, the exception is the user
 * to ignore in the list. The list returned contains (char *)'s */
struct list *user_whoelse(struct list *users, struct user *exception);

/* Return a list of users for the whoelsesince command, the execption is the
 * user to ignore in the list. The list returned contains (char *)'s */
struct list *user_whoelsesince(
    struct list *users, struct user *exception, time_t off_time
);

/* This is used when the user "blocker" wants to block the "victim" */
enum status_code user_block
(
    struct list *users,
    struct user *blocker,
    const char *victim
);

/* The is used when the user "unblocker" wants to unblock the "victim" */
enum status_code user_unblock
(
    struct list *users,
    struct user *unblocker,
    const char *victim
);

/* Return true if the sender is on the receiver's block list, otherwise
 * false is returned */
bool user_on_blocklist (struct user *reciver, struct user *sender);

/* Add the message to the users backlog of messages */
int user_add_to_backlog(struct user *user, const char *name, const char *msg);

/* Return true/false if the two users are equals */
bool user_equal(struct user *user1, struct user *user2);

/* Return the number of items in the user's backlog */
int user_get_backlog_len(struct user *);

/* Pop a backlogged item of the users backlog, if there are no items left then
 * NULL is returned */
struct sdmm_payload *user_pop_backlog(struct user *);

#endif /* USER_H */
