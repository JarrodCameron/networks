/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:33               *
 *                                         *
 *******************************************/

/* Unique identifier for the server */
#define I_AM_SERVER

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

#include "header.h"
#include "logger.h"
#include "slogin.h"
#include "status.h"
#include "user.h"
#include "util.h"

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
static void free_conn (struct connection *conn);
static void *init_conn_handle (void *);

// This will be used later
UNUSED static void *(*handler[])(void*) = {
    [client_init_conn]     = init_conn_handle,
    [server_init_conn]     = NULL,
};

/* Print usage and exit */
static void usage (void)
{
    fprintf(stderr, "Usage: ./server <server_port> <block_duration> <timeout>\n");
    exit(1);
}

static void *init_conn_handle (void *arg)
{

    struct header *head;
    struct cic_payload *cic;
    int ret;
    struct user *curr_user = NULL;
    struct connection *conn = arg;
    enum status_code code;

    code = login_conn (conn->sock, server.users, conn->payload, &curr_user);
    while (code == bad_uname) {
        ret = get_payload(
            conn->sock,
            &head,
            (void *) &cic
        );
        if (ret < 0) {
            elogs("Error: Internal server error\n");
            free_conn(conn);
            return NULL;
        }
        free (head);
        code = login_conn (conn->sock, server.users, cic, &curr_user);
        free (cic);
    }

    if (code == init_failed) {
        elogs("Communication error\n");
        free_conn(conn);
        return NULL;
    }

    assert(curr_user != NULL);

    printf("We have a valid username\n");

    char *user_name = user_get_uname(curr_user);
    if (user_name == NULL) {
        elogs("Failed to alloc buffer\n");
        free_conn(conn);
        return NULL;
    }

    code = auth_user(conn->sock, curr_user);
    switch (code) {
        case comms_error:
            elogs("Failed to communicate to user: \"%s\"\n", user_name);
            free_conn(conn);
            free (user_name);
            return NULL;

        case user_blocked:
            elogs("User blocked: \"%s\"\n", user_name);
            free (user_name);
            free_conn(conn);
            return NULL;

        case init_success:
            logs("User logged in: \"%s\"\n", user_name);
            free (user_name);
            free_conn (conn);
            break;

        case already_on:
            logs("User logged in twice: \"%s\"\n", user_name);
            free (user_name);
            free_conn (conn);
            return NULL;

        default:
            free(user_name);
            free_conn (conn);
            panic("Invalid response while authenticating user, received \"%s\"(%d)\n",
                code_to_str(code),
                code
            );

    }

    return NULL;
}

/* Initialise the users list from the CRED_LIST file */
static int init_users (void)
{
    char uname[MAX_UNAME];
    char pword[MAX_PWORD];

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

static int ptr_cmp (void *a, void *b)
{
    return (a - b);
}

/* Probe handler list to dispatch even and spawn new thread */
static int dispatch_event (struct connection *conn)
{

    enum task_id id = conn->head->task_id;
    if (id != client_init_conn)
        return -1;

    pthread_t *tid = malloc(sizeof(pthread_t));
    if (tid == NULL)
        return -1;

    assert(server.threads != NULL);
    if (list_add(server.threads, tid) < 0) {
        free (tid);
        return -1;
    }

    if (pthread_create(tid, NULL, init_conn_handle, conn) != 0) {
        list_rm(server.threads, tid, ptr_cmp);
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

        logs("We have a connection!!!\n");

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
            elogs("Failed to server request\n");
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
        elogs("Failed to initialise users list\n");
        elogs("Does \"" CRED_LIST "\" exist?\n");
        usage();
    }

    if (init_server () < 0) {
        elogs("Failed to initialise server\n");
        free_users();
        return 1;
    }

    run_server();

    return 0;
}
