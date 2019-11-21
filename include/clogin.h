#ifndef CLOGIN_H
#define CLOGIN_H

/* This contains all of the logic and implementation to log into the server
 * and handling any interataction during the login process for the client to
 * the server.
 *
 * This is the counter part to the slogin.* files
 */

/* Attempt to login into the server, the sock is the socket (fd) to
 * to communicate to the server (TCP).
 * Return:
 *   0  -> Success, client is logged on.
 *   -1 -> Client can't be logged on for what ever reason.
 */
int attempt_login (int sock);

#endif /* CLOGIN_H */
