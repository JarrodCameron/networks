/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   14/11/19 14:49               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include "util.h"
#include "synch.h"
#include "user.h"

struct connection {             /* Contains all information for
                                 * client/server communications */
    int sock;                   /* Socket for this connection */
    struct cic_payload cic;     /* Data deilivered by client */
    pthread_t *thread;          /* The thread handling this connection */
    struct user *user;          /* The user on the other side */
    struct lock *lock;          /* Just in case... shouldn't need it */

    struct in_addr addr;        /* The IPv4 address */
    unsigned int port;          /* Port num for this connection */
};

/* Helper functions */
static int send_broad_logoff(int sock, struct user *user);
static void num_blocked_iter(void *item, void *arg);
static bool conn_user_blocked(struct connection *conn, struct user *user);
static int send_broadcast_logon(struct connection *conn, struct user *new_user);
static int deploy_logon_payload(int sock, struct user *user);
static void deploy_logoff(void *item, void *arg);
static int send_broadcast_msg(struct connection *, struct user *, char *);
static bool valid_broadcast(struct connection *conn, struct user *user);

struct connection *conn_init(void)
{
    struct connection *conn = malloc(sizeof(struct connection));
    if (conn == NULL)
        return conn;

    *conn = (struct connection) {0};

    // To represent unitialised connection
    conn->sock = -1;

    conn->lock = lock_init();
    if (conn->lock == NULL) {
        free(conn->thread);
        free(conn);
        return NULL;
    }

    return conn;
}

void conn_free(struct connection *conn)
{
    if (conn == NULL)
        return;

    struct lock *lock = conn->lock;
    lock_acquire(lock);

    if (conn->sock >= 0)
        close(conn->sock);

    if (conn->thread != NULL)
        free(conn->thread);

    if (conn->user != NULL) {
        // This is a reference, thus don't touch the user
    }

    free(conn);

    lock_release(lock);
    lock_free(lock);
}

int conn_get_sock(struct connection *conn)
{
    assert(conn != NULL);
    int sock;
    lock_acquire(conn->lock);
    sock = conn->sock;
    lock_release(conn->lock);
    assert(sock >= 0);
    return sock;
}

void conn_set_sock(struct connection *conn, int sock)
{
    assert(conn != NULL);
    lock_acquire(conn->lock);
    assert(conn->sock == -1);
    conn->sock = sock;
    lock_release(conn->lock);
}

void conn_set_user(struct connection *conn, struct user *user)
{
    assert(conn != NULL);
    assert(user != NULL);
    lock_acquire(conn->lock);
    assert(conn->user == NULL);
    conn->user = user;
    lock_release(conn->lock);
}

void conn_set_in_addr(struct connection *conn, struct in_addr in)
{
    assert(conn != NULL);
    lock_acquire(conn->lock);
    conn->addr = in;
    lock_release(conn->lock);
}

unsigned short conn_get_port(struct connection *conn)
{
    unsigned short port;
    assert(conn);
    lock_acquire(conn->lock);
    port = conn->port;
    lock_release(conn->lock);
    return port;
}

void conn_set_port(struct connection *conn, unsigned short port)
{
    assert(conn);
    lock_acquire(conn->lock);
    conn->port = port;
    lock_release(conn->lock);
}

struct in_addr conn_get_in_addr(struct connection *conn)
{
    struct in_addr addr;
    assert(conn);
    lock_acquire(conn->lock);
    addr = conn->addr;
    lock_release(conn->lock);
    return addr;
}

pthread_t *conn_get_thread(struct connection *conn)
{
    pthread_t *ret;
    lock_acquire(conn->lock);
    ret = conn->thread;
    lock_release(conn->lock);
    assert(ret != NULL);
    return ret;
}

void conn_set_thread(struct connection *conn, pthread_t *tid)
{
    assert(conn != NULL);
    assert(tid != NULL);

    lock_acquire(conn->lock);
    assert(conn->thread == NULL);
    conn->thread = tid;
    lock_release(conn->lock);
}

void conn_set_cic(struct connection *conn, struct cic_payload cic)
{
    lock_acquire(conn->lock);
    conn->cic = cic;
    lock_release(conn->lock);
}

int conn_broad_log_on(struct list *conns, struct user *user)
{
    struct iter *iter = list_iter_init(conns);
    if (iter == NULL)
        return -1;

    while (iter_has_next(iter) == true) {
        struct connection *conn = iter_get(iter);
        if (conn_user_blocked(conn, user) == false) {
            if (send_broadcast_logon(conn, user) < 0) {
                iter_free(iter);
                return -1;
            }
        }
        iter_next(iter);
    }

    iter_free(iter);

    return 0;
}

int conn_broad_log_off(struct list *conns, struct user *user)
{
    char *name = user_get_uname(user);
    if (name == NULL)
        return -1;

    list_traverse(conns, deploy_logoff, user);

    free(name);
    return 0;
}

int conn_broad_msg
(
    struct list *conns,
    struct user *user,
    char msg[MAX_MSG_LENGTH]
)
{
    struct iter *iter = list_iter_init(conns);
    if (iter == NULL)
        return -1;

    while (iter_has_next(iter) == true) {
        struct connection *conn = iter_get(iter);

        if (send_broadcast_msg(conn, user, msg) < 0) {
            iter_free(iter);
            return -1;
        }

        iter_next(iter);
    }
    iter_free(iter);
    return 0;
}

int conn_get_by_user(struct list *conns, struct user *user, struct connection **ret)
{
    struct iter *iter = list_iter_init(conns);
    if (iter == NULL)
        return -1;

    while (iter_has_next(iter) == true) {
        struct connection *conn = iter_get(iter);
        lock_acquire(conn->lock);
        if (user_equal(conn->user, user) == true) {
            *ret = conn;
            lock_release(conn->lock);
            return 0;
        }
        lock_release(conn->lock);
        iter_next(iter);
    }

    iter_free(iter);
    *ret = NULL;
    return 0;
}

int conn_get_num_blocked(struct list *conns, struct user *user)
{
    int ret = 0;
    struct tuple tuple = {
        .items[0] = &ret,
        .items[1] = user,
    };

    list_traverse(conns, num_blocked_iter, &tuple);
    return ret;
}

/* Return if the user was the block list of the conn */
static void num_blocked_iter(void *item, void *arg)
{
    struct connection *conn = item;
    struct tuple *tuple = arg;

    int *ret_ptr = tuple->items[0];
    struct user *user = tuple->items[1];

    if (conn_user_blocked(conn, user) == true)
        *ret_ptr += 1;
}

/* Return true if the user is blocked by the conn->user */
static bool conn_user_blocked(struct connection *conn, struct user *user)
{
    bool ret;
    lock_acquire(conn->lock);
    ret = user_on_blocklist(conn->user, user);
    lock_release(conn->lock);
    return ret;
}

/* Used to iterate over the list of connection and notify each one
 * that the user has logged of */
static void deploy_logoff(void *item, void *arg)
{
    struct connection *conn = item;
    struct user *user = arg;
    lock_acquire(conn->lock);

    if (valid_broadcast(conn, user) == false) {
        lock_release(conn->lock);
        return;
    }

    send_broad_logoff(conn->sock, user);
    lock_release(conn->lock);
}

/* Simple send the log off message via the sock, return -1 on error, otherwise
 * 0 is returned */
static int send_broad_logoff(int sock, struct user *user)
{
    int ret;
    if (send_payload_scmd(sock, broad_logoff, 0 /* ignored */) < 0)
        return -1;

    char *name = user_get_uname(user);
    if (name == NULL)
        return -1;

    ret = send_payload_sbof(sock, name);
    free(name);
    return ret;
}

/* Used to send the broadcast payload to a particular user to show that
 * the "msg" has been sent */
static int send_broadcast_msg
(
    struct connection *conn,
    struct user *user,
    char *msg
)
{
    lock_acquire(conn->lock);

    if (valid_broadcast(conn, user) == false) {
        lock_release(conn->lock);
        return 0;
    }

    if (send_payload_scmd(conn->sock, broad_msg, 0 /* ignored */) < 0) {
        lock_release(conn->lock);
        return -1;
    }

    if (send_payload_sbm(conn->sock, msg) < 0) {
        lock_release(conn->lock);
        return -1;
    }

    lock_release(conn->lock);
    return 0;
}

/* Return true if the "user" is valid enought to broadcast the the user on
 * the "conn" side */
static bool valid_broadcast(struct connection *conn, struct user *user)
{
    if (user_is_logged_on(conn->user) == false)
        return false;

    if (user_equal(conn->user, user) == true)
        return false;

    if (user_on_blocklist(conn->user, user) == true)
        return false;

    return true;
}

/* Used to send the broadcast payload to a particular user to show that
 * "new_user" is logged in */
static int send_broadcast_logon(struct connection *conn, struct user *new_user)
{
    lock_acquire(conn->lock);

    if (valid_broadcast(conn, new_user) == false) {
        lock_release(conn->lock);
        return 0;
    }

    if (deploy_logon_payload(conn->sock, new_user) < 0) {
        lock_release(conn->lock);
        return -1;
    }

    lock_release(conn->lock);
    return 0;
}

/* Send the name of the new user to the client for them to handle */
static int deploy_logon_payload(int sock, struct user *user)
{
    if (send_payload_scmd(sock, broad_logon, 0 /* ignored */) < 0) {
        return -1;
    }

    char *name = user_get_uname(user);
    if (name == NULL)
        return -1;

    if (send_payload_sbon(sock, name) < 0) {
        free(name);
        return -1;
    }

    free(name);
    return 0;
}
