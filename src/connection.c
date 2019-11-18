/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   14/11/19 14:49               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include "connection.h"
#include "header.h"
#include "status.h"
#include "synch.h"
#include "user.h"

struct connection {             /* Contains all information for
                                 * client/server communications */
    int sock;                   /* Socket for this connection */
    struct cic_payload cic;     /* Data deilivered by client */
    pthread_t *thread;          /* The thread handling this connection */
    struct user *user;          /* The user on the other side */
    struct lock *lock;          /* Just in case... shouldn't need it */
};

/* Helper functions */
static bool valid_log_on(struct connection *conn, struct user *user);
static bool conn_user_blocked(struct connection *conn, struct user *user);
static int send_broadcast_logon(struct connection *conn, struct user *new_user);
static int deploy_logon_payload(int sock, struct user *user);
static void deploy_logoff(void *item, void *arg);
static int send_broadcast_msg(struct connection *, struct user *, char *);
static bool valid_broad_msg(struct connection *conn, struct user *user);

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
        if (send_broadcast_logon(conn, user) < 0) {
            iter_free(iter);
            return -1;
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

    list_traverse(conns, deploy_logoff, name);

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

        if (conn_user_blocked(conn, user) == false) {
            if (send_broadcast_msg(conn, user, msg) < 0) {
                iter_free(iter);
                return -1;
            }
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
    char *arg_name = arg;

    lock_acquire(conn->lock);

    char *name = user_get_uname(conn->user);
    if (name == NULL) {
        lock_release(conn->lock);
        return;
    }

    if (strncmp(name, arg_name, MAX_UNAME) == 0) {
        free(name);
        lock_release(conn->lock);
        return;
    }
    free(name);

    if (send_payload_scmd(conn->sock, broad_logoff, 0 /* ignored */) < 0) {
        lock_release(conn->lock);
        return;
    }

    send_payload_sbof(conn->sock, arg_name);
    lock_release(conn->lock);
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

    if (valid_broad_msg(conn, user) == false) {
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

/* Return true if the "conn" can be used to send the broadcast message */
static bool valid_broad_msg(struct connection *conn, struct user *user)
{
    if (user_is_logged_on(conn->user) == false)
        return false;

    if (user_equal(conn->user, user) == true)
        return false;

    return true;
}

/* Return true if this connection should be used to broadcast that the user
 * is online, otherwise return false */
static bool valid_log_on(struct connection *conn, struct user *user)
{
    if (user_equal(conn->user, user) == true)
        return false;

    if (user_is_logged_on(conn->user) == false)
        return false;

    return true;
}

/* Used to send the broadcast payload to a particular user to show that
 * "new_user" is logged in */
static int send_broadcast_logon(struct connection *conn, struct user *new_user)
{
    lock_acquire(conn->lock);

    if (valid_log_on(conn, new_user) == false) {
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
