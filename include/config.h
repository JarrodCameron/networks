#ifndef CONFIG_H
#define CONFIG_H

/* This file contains constants for configuration the executation style of
 * the program.
 */

/* Flieds for user.[ch] */
#define MAX_UNAME (128)
#define MAX_PWORD (128)

/* Size of the backlog for the server, i.e. can only queue this many
 * connections. If the server is too slow increase this number */
#define SERVER_BACKLOG (20)

/* Where to store the list of credentials */
#define CRED_LIST "credentials.txt"

/* The maximium length that a user can send from the command line */
#define MAX_MSG_LENGTH (1024)

#endif /* CONFIG_H */
