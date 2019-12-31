#ifndef CONFIG_H
#define CONFIG_H

/* Settings that may be of interest to power users */

/* Flieds for user.[ch] */
#define MAX_UNAME (128)
#define MAX_PWORD (128)

/* Size of the backlog for the server, i.e. can only queue this many
 * connections. If the server is too slow increase this number */
#define SERVER_BACKLOG (20)

/* Size of the backlog for the client. This is how many peers can be in
 * the backlog for peer to peer connections */
#define CLIENT_BACKLOG (20)

/* Where to store the list of credentials */
#define CRED_LIST "credentials.txt"

/* The maximum length that a user can send from the command line */
#define MAX_MSG_LENGTH (1024)

/* The maximum length of a command that can be entered by the user */
#define MAX_COMMAND (1024)

/* Uncomment the following line to enable the logging feature. Currently
 * logging output to the console is disabled, hence the comment */
//#define DISABLE_LOGGING

/* this is the number of login attempts before the user gets blocked,
 * spec says three. */
#define NLOGIN_ATTEMPTS 3

#endif /* CONFIG_H */
