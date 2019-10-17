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
 *   1  -> Login successful
 *   0  -> Login failed
 *   <0 -> Error during login process (i.e. closed connection)
 */
int attempt_login (int sock);

#endif /* CLOGIN_H */
