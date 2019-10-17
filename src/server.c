/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:33               *
 *                                         *
 *******************************************/

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

#include "util.h"
#include "user.h"
#include "list.h"
#include "config.h"
#include "header.h"
#include "slogin.h"

static struct {
    uint32_t block_duration;    /* How long to block a user */
    int port;                   /* Port to listen to connections */
    int timeout;                /* How long until a user gets kicked */
    struct list *users;         /* List of all valid users */
    int listen_sock;            /* The socket to listen on (i.e. the fd) */
    struct list *threads;       /* List of each spawned thread */
} server = {0};

struct connection {
    int sock;                   /* Socket for this connection */
    struct header *head;        /* Information about the client/task */
    void *payload;              /* Data deilivered by client */
};

/* Helper functions */
static void usage (void);
static int init_users (void);
static int init_args (const char *port, const char *dur, const char *timeout);
static int init_server (void);
static void free_users (void);
static int dispatch_event (struct connection *conn);
static struct connection *init_conn (void);
static void *login_handle (void *);
static void free_conn (struct connection *conn);

static void *(*handler[])(void*) = {
    [client_login_attempt] = login_handle,
    [server_login_reply]   = NULL,
};

/* Print usage and exit */
static void usage (void)
{
    fprintf(stderr, "Usage: ./server <server_port> <block_duration> <timeout>\n");
    exit(1);
}

/* Initialise the users list from the CRED_LIST file */
static int init_users (void)
{
    char uname[128];
    char pword[128];

    server.users = list_init();
    if (server.users == NULL)
        return -1;

    FILE *f = fopen(CRED_LIST, "r");

    while (fscanf(f, "%s %s", uname, pword) == 2) {
        struct user *user = init_user(uname, pword);
        if (user == NULL)
            goto init_users_fail;

        if (list_add(server.users, user) < 0)
            goto init_users_fail;
    }
    fclose(f);
    return 0;

init_users_fail:
    free_users();
    fclose(f);
    return -1;
}

/* Initialise all of the user arguments, return -1 if invalid argument */
static int init_args (const char *port, const char *dur, const char *timeout)
{
    sscanf(port, "%d", &(server.port));
    if (server.port < 1024)
        return -1;

    sscanf(timeout, "%d", &(server.timeout));
    if (server.timeout < 0)
        return -1;

    sscanf(dur, "%d", &(server.block_duration));

    return 0;
}

/* Initialise the server, connect and bind to port */
static int init_server (void)
{
    int ret = 0;

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(server.port); // Port
    server.listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    server.threads = list_init();
    if (server.threads == NULL)
        return -1;

    if (server.listen_sock < 0)
        return -1;

    ret = bind(
        server.listen_sock,
        (struct sockaddr *) &server_address,
        sizeof(server_address)
    );
    if (ret < 0) {
        close(server.listen_sock);
        return -1;
    }

    ret = listen(
        server.listen_sock,
        SERVER_BACKLOG
    );
    if (ret < 0) {
        close(server.listen_sock);
        return -1;
    }

    return 0;
}

/* Free the list of users from memory (memory leaks are bad) */
static void free_users (void)
{
    if (server.users == NULL)
        return;
    list_free(server.users, (void*) free_user);
}

/* Probe handler list to dispatch even and spawn new thread */
static int dispatch_event (struct connection *conn)
{

    int id = conn->head->task_id;
    if (id > (int)ARRSIZE(handler) || handler[id] == NULL)
        return -1;

    pthread_t *tid = malloc(sizeof(pthread_t));
    if (tid == NULL)
        return -1;

    assert(server.threads != NULL);
    if (list_add(server.threads, tid) < 0) {
        free (tid);
        return -1;
    }

    if (pthread_create(tid, NULL, handler[id], conn) != 0) {
        free (tid);
        return -1;
    }

    return 0;
}

/* Initialise and reutrn a connection struct */
static struct connection *init_conn (void)
{
    struct connection *conn;
    conn = malloc(sizeof(struct connection));
    if (!conn)
        return NULL;
    *conn = (struct connection) {0};

    // To indicate invalid socket
    conn->sock = -1;
    return conn;
}

/* Handle login attempts by the user, this runs in a seperate thread to the
 * main server */
static void *login_handle(void *arg)
{
    struct connection *conn = arg;
    struct cla_payload *cla = conn->payload;

    if (server_auth (conn->sock, server.users, cla) < 0) {
        fprintf(stderr, "Error authenticating user\n");
    } else {
        // Ideally we would start a loops to listen the future messages from the client.
    }

    return NULL;
}

/* Free the connection and it's state, close any fd's */
static void free_conn (struct connection *conn)
{
    if (!conn)
        return;

    if (conn->sock >= 0)
        close (conn->sock);

    if (conn->head)
        free (conn->head);

    if (conn->payload)
        free(conn->payload);

    free(conn);
}

/* Where the actual magic happens.
 * Since the server must keep running it never returns. */
static void run_server (void)
{
    int sock;
    struct connection *conn;
    struct sockaddr_in client_addr = {0};
    socklen_t client_addr_len = sizeof(client_addr);
    while (1) {
        sock = accept(
            server.listen_sock,
            (struct sockaddr *) &client_addr,
            &client_addr_len
        );
        if (sock < 0) {
            perror("accept: ");
            continue;
        }

        printf("We have a connection!!!\n");

        conn = init_conn ();
        if (conn == NULL) {
            free_conn(conn);
            continue;
        }
        conn->sock = sock;

        if (get_payload(conn->sock, &(conn->head), &(conn->payload)) < 0) {
            free_conn(conn);
            continue;
        }

        if (dispatch_event (conn) < 0) {
            fprintf(stderr, "Failed to server request\n");
            free_conn(conn);
            continue;
        }

        // The conn object is dispatched to a new thread and is no longer
        // our responsibility.
        conn = NULL;
    }
}

int main (int argc, char **argv)
{

    if (argc != 4) {
        fprintf(stderr, "Invalid number of arguments\n");
        usage ();
    }

    if (init_args (argv[1], argv[2], argv[3]) < 0) {
        fprintf(stderr, "Invalid argument(s)\n");
        usage();
    }

    if (init_users () < 0) {
        fprintf(stderr, "Failed to initialise users list\n");
        fprintf(stderr, "Does \"" CRED_LIST "\" exist?\n");
        usage();
    }

    if (init_server () < 0) {
        fprintf(stderr, "Failed to initialise server\n");
        free_users();
        return 1;
    }

    run_server();

    return 0;
}
