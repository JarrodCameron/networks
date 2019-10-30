#ifndef SLOGIN_H
#define SLOGIN_H

/* slogin.h and slogin.c contains all of the logic for the logic for the
 * server to login in and handle login requests.
 *
 * All logic for the client to login is stored in clogin.h and clogin.c
 */

#include "status.h"
#include "user.h"

/* This function will simply do all of the stuff to check the username.
 * If NULL is returned the current thread may quit. If the user is valid then
 * the current user is returned in curr_user and the appropriate status code
 * is returned. */
enum status_code login_conn
(
    int sock,
    struct list *users,
    struct cic_payload *cic,
    struct user **curr_user
);

/* This function is used to authenticate the user simply by asking for the
 * password. This handles any conditions associated with logging in the user
 * (i.e. blocking on N invalid attempts) */
enum status_code auth_user(int sock, struct user *user);

#endif /* SLOGIN_H */
