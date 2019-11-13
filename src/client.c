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
#include <sys/select.h>
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

/* Helper functions */
static void usage (void);
static int init_args (const char *ip_addr, const char *port);
static int conn_to_server (int sock);
static int init_connection (void);
static int handle_client_timeout(void);
static int handle_scmd(struct scmd_payload *scmd);
static int socket_read_handle(void);
static int stdin_read_handle(void);
static int deploy_command(char cmd[MAX_COMMAND]);
static int dual_select(int *sock_ret, int *stdin_ret);
static void run_client (void);
static int cmd_whoelse(struct scmd_payload *scmd);

/* The name of each command and the respective handle */
struct {
    const char *name;
    int (*handle)(struct scmd_payload *);
} commands[] = {
    // WARNING: Do not change the order!!!
    {.name = "message",      .handle = NULL},
    {.name = "broadcast",    .handle = NULL},
    {.name = "whoelsesince", .handle = NULL},
    {.name = "whoelse",      .handle = cmd_whoelse},
    {.name = "block",        .handle = NULL},
    {.name = "unblock",      .handle = NULL},
    {.name = "logout",       .handle = NULL},
    {.name = "startprivate", .handle = NULL},
    {.name = "private",      .handle = NULL},
    {.name = "stopprivate",  .handle = NULL},
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

        case bad_command:
            printf("You entered an invalid command! <\n");
            return 0;

        default:
            panic("Received unknown server command: \"%s\"(%d)\n",
                code_to_str(scmd->code), scmd->code
            );
    }
}

/* This is called when the main loop receives a message from the socket
 * and needs to be handled */
static int socket_read_handle(void)
{
    struct header head;
    void *payload;
    int ret;

    ret = get_payload(client.sock, &head, &payload);
    if (ret < 0)
        return -1;

    if (head.task_id == server_command)
        handle_scmd(payload);

    free(payload);
    return 0;
}

/* This is called when the main loop receives a message from standard input
 * and needs to be handled */
static int stdin_read_handle(void)
{
    char cmd[MAX_COMMAND] = {0};

    int ret = read(0, cmd, MAX_COMMAND-1);

    if (ret <= 1)
        return 0;

    if (cmd[ret-1] == '\n' || cmd[ret-1] == '\r')
        cmd[ret-1] = '\0';

    if (deploy_command(cmd) < 0) {
        printf("Failed to send command: \"%s\"\n", cmd);
        return -1;
    }
    return 0;
}

/* Used for when the client sends to whoelse command to the server */
static int cmd_whoelse(struct scmd_payload *scmd)
{
    struct sw_payload sw = {0};
    int ret;

    uint64_t num_users = scmd->extra;

    if (num_users == 1) {
        printf("There is 1 user online\n");
    } else {
        printf("There are %ld users online\n", num_users);
    }

    uint64_t i;
    for (i = 0; i < num_users; i++) {
        ret = recv_payload_sw(client.sock, &sw);
        if (ret < 0)
            return -1;

        printf("%ld. %s\n", i+1, sw.username);
    }

    return 0;
}

/* Return the first legit character of a line. Non-legit characters are spaces
 * and null bytes */
static const char *get_first_non_space(const char *line)
{
    const char *ret = line;
    while (*ret == ' ' || *ret == '\0')
        ret++;

    if (*ret == '\0')
        return NULL;

    return ret;
}

/* Send the command to the server and handle the response appropriately */
static int deploy_command(char cmd[MAX_COMMAND])
{
    struct scmd_payload scmd = {0};

    if (send_payload_ccmd(client.sock, cmd) < 0)
        return -1;

    if (recv_payload_scmd(client.sock, &scmd) < 0) {
        return -1;
    }

    switch (scmd.code) {
        case server_error:
            printf("Internal server error!\n");
            return -1;

        case bad_command:
            printf("Invalid command: \"%s\"\n", cmd);
            return 0;

        case task_ready:
            /* The task is ready to be handled */
            break;

        default:
            panic("Unknown error code: \"%s\"(%d)\n",
                code_to_str(scmd.code), scmd.code
            );
    }

    const char *start = get_first_non_space(cmd);
    if (start == NULL)
        return 0;

    unsigned int i;
    for (i = 0; i < ARRSIZE(commands); i++) {
        const char *name = commands[i].name;
        if (strncmp(start, name, strlen(name)-1) == 0) {
            return commands[i].handle(&scmd);
        }
    }

    panic("The server should of returned \"bad_command\"!\n");
}

/* Use the "select" system call for stdin and the server socket. If the
 * fd is selected then it is set to 1, otherwise it is set to zero. Return
 * -1 on error */
static int dual_select(int *sock_ret, int *stdin_ret)
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(0, &read_set); /* stdin fd */
    FD_SET(client.sock, &read_set);

    int nfds = MAX(0 /* stdin fd */, client.sock) + 1;

    int ret = select(nfds, &read_set, NULL, NULL, NULL);
    if (ret < 0) {
        return -1;
    }

    *sock_ret = !!FD_ISSET(client.sock, &read_set);
    *stdin_ret = !!FD_ISSET(0 /* stdin fd */, &read_set);
    return 0;
}

/* Run the client, this is where the communication to the server takes place.
 * This is logs the client in to the server */
static void run_client (void)
{
    int sock_ret, stdin_ret;

    if (attempt_login(client.sock) < 0)
        return;

    while (1) {

        printf("> ");
        fflush(stdout);

        sock_ret = stdin_ret = -1;
        if (dual_select(&sock_ret, &stdin_ret) < 0) {
            printf("Failed to receive input from server or client\n");
            return;
        }

        if (sock_ret == 1) {
            if (socket_read_handle() < 0) {
                printf("Failed to handle socket payload\n");
                return;
            }
        }

        if (stdin_ret == 1) {
            if (stdin_read_handle() < 0) {
                printf("Failed to handle stdin payload\n");
                return;
            }
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
