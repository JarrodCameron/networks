#ifndef USER_H
#define USER_H

#include <stdint.h>

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

#endif /* USER_H */
