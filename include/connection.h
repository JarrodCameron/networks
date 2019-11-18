#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "header.h"
#include "status.h"
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

/* Broadcast "msg" to all active connections, execpted for the "user" (since
 * it is pointless to send the same message to them self) */
int conn_broad_msg(struct list *conns, struct user *, char msg[MAX_MSG_LENGTH]);

/* Return the connection for the respective user. Return -1 on error, otherwise
 * return 0. */
int conn_get_by_user(struct list *conns, struct user *user, struct connection **ret);

#endif /* CONNECTION_H */
