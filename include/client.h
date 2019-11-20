#ifndef CLIENT_H
#define CLIENT_H

#include "config.h"
#include "list.h"

/* Return the list of peer to peer connections */
struct list *client_get_ptops(void);

/* Return the socket that the client uses to communicate to the server */
int client_get_server_sock(void);

/* return the socket that the client uses to accept other peers */
int client_get_ptop_sock(void);

/* Set the username of the client */
void client_set_uname(const char name[MAX_UNAME]);

/* Return a copy of the user name inside of a buffer MAX_UNAME bytes long.
 * The name needs to be free()'d by the caller */
char *client_get_uname(void);

#endif /* CLIENT_H */
