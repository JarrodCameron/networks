/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:35               *
 *                                         *
 *******************************************/

/* Questions:
 * - Do we have to worry about receiving messages in the wrong order? - Ask webcms
 * - On failed attempts can we exit or do we need to log out? - Just exit
 * - Multiple threads for client? - Nah, just use select(2)
 */

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

#include "clogin.h"
#include "config.h"
#include "header.h"
#include "util.h"

struct {
    struct sockaddr_in sockaddr;
    int sock;
    pthread_t child_thread;
} client = {0};

/* Print usage and exit */
static void usage (void)
{
    fprintf (stderr, "Usage: ./client <server_ip> <server_port>\n");
    exit (1);
}

/* Initialise the arguments, return -1 on error */
static int init_args (const char *ip_addr, const char *port)
{
    int dummy;
    struct sockaddr_in server_address = {0};

    // string to legin ipv4
    if (inet_pton(AF_INET, ip_addr, &(server_address.sin_addr)) != 1)
        return -1;

    sscanf(port, "%d", &dummy);
    if (dummy < 1024)
        return -1;
    server_address.sin_port = htons(dummy);

    server_address.sin_family = AF_INET;

    client.sockaddr = server_address;

    return 0;
}

/* This handles the application layer protocol to the server */
static int conn_to_server (int sock)
{
    struct sic_payload sic = {0};

    if (send_payload_cic(sock, init_success) < 0)
        return -1;

    if (recv_payload_sic(sock, &sic) < 0)
        return -1;

    if (sic.code != init_success)
        return -1;

    return 0;
}

/* Initialise the connection to the server for communictions.
 * This does NOT count the login process */
static int init_connection (void)
{
    int sock, ret;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    ret = connect(
        sock,
        (struct sockaddr *) &(client.sockaddr),
        sizeof(client.sockaddr)
    );
    if (ret < 0) {
        close (sock);
        return -1;
    }

    if (conn_to_server(sock) < 0) {
        close (sock);
        return -1;
    }

    client.sock = sock;
    return 0;
}

/* The client has timed out and needs to be logged off */
static int handle_client_timeout(void)
{
    printf("You have timed out!\n");
    printf("Logging off now.\n");
    exit(1); // Can we just shut down or do we need to log off?
    return -1;
}

/* A command was received by the server, handle the command from here */
static int handle_scmd(struct scmd_payload *scmd)
{
    switch (scmd->code) {
        case time_out:
            return handle_client_timeout();

        default:
            panic("Received unknown server command: \"%s\"(%d)\n",
                code_to_str(scmd->code), scmd->code
            );
    }
}

/* This is where the listening thread starts, just wating for a message from
 * the server. */
static void *listen_thread_landing(UNUSED void *arg)
{
    struct header head;
    void *payload;
    int ret;

    ret = get_payload(client.sock, &head, &payload);
    if (ret < 0)
        return NULL;

    if (head.task_id == server_command)
        handle_scmd(payload);

    free(payload);
    return NULL;
}

/* This will spawn a listening thread for the server, this is used to wait for
 * messegaes (i.e. timeout). */
static int spawn_listener()
{
    return pthread_create(
        &(client.child_thread),
        NULL,
        listen_thread_landing,
        NULL
    );
}

/* Send the command to the server and handle the response appropriately */
static int deploy_command(char cmd[MAX_COMMAND])
{
    return send_payload_ccmd(client.sock, cmd);
}

/* Run the client, this is where the communication to the server takes place.
 * This is logs the client in to the server */
static void run_client (void)
{
    if (attempt_login(client.sock) < 0)
        return;

    if (spawn_listener() < 0) {
        // TODO:
        // 1. Spawn a listening thread for timeout signals.
        // 2. upon error disonnect
    }

    while (1) {
        char cmd[MAX_COMMAND] = {0};

        printf("> ");
        fflush(stdout);

        int ret = read(0, cmd, MAX_COMMAND-1);

        if (ret <= 1)
            continue;

        if (cmd[ret-1] == '\n' || cmd[ret-1] == '\r')
            cmd[ret-1] = '\0';

        if (deploy_command(cmd) < 0) {
            printf("Failed to send command: \"%s\"\n", cmd);
        }
    }

}

int main (int argc, char **argv)
{

    if (argc != 3) {
        usage ();
    }

    if (init_args (argv[1], argv[2]) < 0) {
        fprintf(stderr, "Invalid argument(s)\n");
        usage();
    }

    if (init_connection () < 0) {
        fprintf(stderr, "Failed to initialise connection\n");
        fprintf(stderr, "Did you use the right IP addres and port?\n");
        return 1;
    }

    run_client ();

    return 0;
}

