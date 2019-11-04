#ifndef SLOGIN_H
#define SLOGIN_H

/* slogin.h and slogin.c contains all of the logic for the logic for the
 * server to login in and handle login requests.
 *
 * All logic for the client to login is stored in clogin.h and clogin.c
 */

#include "status.h"
#include "user.h"

/* This function intiialises the connection to the client from the server's
 * perspective. Returnt the logged in user on success, return NULL if the user
 * can not be authenticated. The calling thread will quit when NULL is
 * returned. The list is needed to query existing users in the database */
struct user *auth_user(int sock, struct list *users);

#endif /* SLOGIN_H */
