#ifndef USER_H
#define USER_H

#include "config.h"

struct user;

/* Given the username and password, create and return a user. */
struct user *init_user (const char uname[MAX_UNAME], const char pword[MAX_PWORD]);

/* Free the user from memory */
void free_user (struct user *user);

#endif /* USER_H */
