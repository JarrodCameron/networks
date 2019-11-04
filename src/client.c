/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:35               *
 *                                         *
 *******************************************/

/* Unique identifies for the client */
#define I_AM_CLIENT

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

#include "clogin.h"
#include "config.h"
#include "header.h"
#include "util.h"

struct {
    struct sockaddr_in sockaddr;
    int sock;
} client = {0};

enum cmd_id {               /* The command to be done by the user, for what
                             * each of these do read the spec (p3/p4) */
    cmd_message      = 1,   /* message <user> <message> */
    cmd_breadcase    = 2,   /* broadcase <message> */
    cmd_whoelse      = 3,   /* whoelse */
    cmd_whoelsesince = 4,   /* whoelsesince <time> */
    cmd_block        = 5,   /* block <user> */
    cmd_unblock      = 6,   /* unblock <user> */
    cmd_logout       = 7,   /* logout */
};

struct command {
    char **tokens;      /* Each argument given by client */
    uint32_t ntokens;   /* Number of words in command */
    enum cmd_id cmd_id; /* The command the user want to do */
};

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

/* Run the client, this is where the communication to the server takes place.
 * This is logs the client in to the server */
static void run_client (void)
{
    if (attempt_login(client.sock) < 0)
        return;

    while (1) {
        char cmd[MAX_COMMAND] = {0};

        printf("> ");
        fflush(stdout);

        int ret = read(0, cmd, MAX_COMMAND-1);

        if (ret <= 1)
            continue;

        if (cmd[ret-1] == '\n' || cmd[ret-1] == '\r')
            cmd[ret-1] = '\0';

        printf("cmd: %s\n", cmd);

        /*
         * /------------------------------------------------------------------\
         * |TODO, this is where we should parse the input, however the server |
         * |needs more work before we can do this.                            |
         * \------------------------------------------------------------------/
         */
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

