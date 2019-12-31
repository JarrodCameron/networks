#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "header.h"
#include "user.h"

/* Defined in connection.c */
struct connection;

/* Initialise the connection */
struct connection *conn_init(void);

/* Free the connection from memory, close socket if open */
void conn_free(struct connection *);

/* Return the socket for the connection, iff already set */
int conn_get_sock(struct connection *);

/* Set the socket for the connection, can only be done once */
void conn_set_sock(struct connection *, int sock);

/* Set the user for the connection, can only be done once */
void conn_set_user(struct connection *, struct user *);

/* Set the in_addr for this connection for later use */
void conn_set_in_addr(struct connection *, struct in_addr);

/* Return the sockaddr/IPv4 address for this connection */
struct in_addr conn_get_in_addr(struct connection *);

/* Return the port number (NOT SOCKET) used for this connection */
unsigned short conn_get_port(struct connection *);

/* Set the port number for this connection */
void conn_set_port(struct connection *, unsigned short port);

/* Return a reference to the thread for this connection */
pthread_t *conn_get_thread(struct connection *);

/* Set the tid for the connection, can only be done once */
void conn_set_thread(struct connection *, pthread_t *);

/* Set the cic_payload for the connection */
void conn_set_cic(struct connection *, struct cic_payload);

/* Broad case that the "user" has logged on, return -1 on error, otherwise
 * return 0 */
int conn_broad_log_on(struct list *conns, struct user *);

/* Broadcast that the "user" has logged off, return -1 on error, otherwise
 * return 0 */
int conn_broad_log_off(struct list *conns, struct user *);

/* Broadcast "msg" to all active connections, except for the "user" (since
 * it is pointless to send the same message to them self) */
int conn_broad_msg(struct list *conns, struct user *, char msg[MAX_MSG_LENGTH]);

/* Return the connection for the respective user. Return -1 on error, otherwise
 * return 0. */
int conn_get_by_user(struct list *conns, struct user *user, struct connection **ret);

/* Return the number of users in the list of connections that block the user */
int conn_get_num_blocked(struct list *conns, struct user *user);

#endif /* CONNECTION_H */
