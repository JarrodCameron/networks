/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:35               *
 *                                         *
 *******************************************/

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

#include "debug.h"
#include "config.h"

struct {
    struct sockaddr_in sockaddr;
} client = {0};

struct command {
    char **tokens;      /* Each argument given by client */
    uint32_t ntokens;   /* Number of words in command */
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
    return 0;
}

/* Attempt to login to the server */
static int attempt_login (void)
{
    // TODO
    return -1;
}

/* Get the command from the user and split it into tokens */
static struct command *get_command (void)
{
    // TODO
    return NULL;
}

/* Free the command from memory */
static void free_command(struct command *cmd)
{
    // TODO
    (void) cmd;
}

/* Run the client, this is where the communication to the server takes place.
 * This is logs the client in to the server */
static void run_client (void)
{
    if (attempt_login () < 0)
        return;

    struct command *cmd = NULL;

    while (1) {
        cmd = get_command();
        if (cmd == NULL) {
            printf("Please enter a valid string\n");
        }

        // TODO switch statement for the command

        free_command(cmd);
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

