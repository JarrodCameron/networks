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

#include "debug.h"
#include "user.h"
#include "list.h"
#include "config.h"
#include "header.h"

static struct {
    uint32_t block_duration;    /* How long to block a user */
    int port;                   /* Port to listen to connections */
    int timeout;                /* How long until a user gets kicked */
    struct list *users;         /* List of all valid users */
    int listen_sock;            /* The socket to listen on (i.e. the fd) */
} server = {0};

struct connection {
    int sock;                   /* Socket for this connection */
    struct header *head;        /* Information about the client/task */
    void *payload;              /* Data deilivered by client */
};

static void (*handler[])(void*) = {
    [client_login_attempt] = NULL,
    [server_login_reply]   = NULL,
};

/* Helper functions */
static void usage (void);
static int init_users (void);
static int init_args (const char *port, const char *dur, const char *timeout);
static int init_server (void);
static void free_users (void);
static int dispatch_event (struct connection *conn);

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

    server.users = init_list();
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
    (void) conn;
    (void) handler;
    // TODO Dispatch list + fork
    return -1;
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

        conn = malloc(sizeof(struct connection));
        if (!conn) {
            perror("malloc: ");
            close(sock);
            continue;
        }

        if (dispatch_event (conn) < 0) {
            fprintf(stderr, "Failed to server request\n");
            free(conn);
            close(sock);
            continue;
        }
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
