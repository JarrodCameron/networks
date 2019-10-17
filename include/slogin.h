#ifndef SLOGIN_H
#define SLOGIN_H

/* slogin.h and slogin.c contains all of the logic for the logic for the
 * server to login in and handle login requests.
 *
 * All logic for the client to login is stored in clogin.h and clogin.c
 */

/* This is called when the client asks the server to be authenticated.
 * This is were all checks are done.
 * The current list of users must be passed in.
 * Return value:
 *   -> 1  = User has logged on successfully.
 *   -> 0  = User has failed to be authenticated.
 *   -> <0 = There was an error during the authentication process (for example,
 *           out of memory).
 */
int server_auth (int sock, struct list *users, struct cla_payload *cla);

#endif /* SLOGIN_H */
