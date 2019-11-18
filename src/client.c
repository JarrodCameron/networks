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
#include <stdbool.h>

#include "clogin.h"
#include "config.h"
#include "header.h"
#include "util.h"

struct {
    struct sockaddr_in sockaddr;
    int sock;
} client = {0};

/* Helper functions */
static int cmd_message(struct scmd_payload *scmd);
static void usage (void);
static int init_args (const char *ip_addr, const char *port);
static int conn_to_server (int sock);
static int init_connection (void);
static NORETURN int handle_client_timeout(void);
static int handle_broad_logon(void);
static int handle_scmd(struct scmd_payload *scmd);
static int socket_read_handle(void);
static int stdin_read_handle(void);
static bool is_help_cmd(char *cmd);
static void cmd_help(void);
static int cmd_whoelse(struct scmd_payload *scmd);
static int cmd_whoelsesince(struct scmd_payload *scmd);
static int cmd_broadcast(UNUSED struct scmd_payload *scmd);
static int cmd_block(struct scmd_payload *scmd);
static const char *get_first_non_space(const char *line);
static int deploy_command(char cmd[MAX_COMMAND]);
static int dual_select(int *sock_ret, int *stdin_ret);
static void run_client (void);

/* The name of each command and the respective handle */
struct {
    const char *name;
    int (*handle)(struct scmd_payload *);
} commands[] = {
    // WARNING: Do not change the order!!!
    {.name = "message",      .handle = cmd_message},
    {.name = "broadcast",    .handle = cmd_broadcast},
    {.name = "whoelsesince", .handle = cmd_whoelsesince},
    {.name = "whoelse",      .handle = cmd_whoelse},
    {.name = "block",        .handle = cmd_block},
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
static NORETURN int handle_client_timeout(void)
{
    printf("You have timed out!\n");
    printf("Logging off now.\n");
    exit(1);
}

/* When a different client logs on their name is broadcasted, this is the
 * handler for when that happens */
static int handle_broad_logon(void)
{
    struct sbon_payload sbon = {0};
    if (recv_payload_sbon(client.sock, &sbon) < 0)
        return -1;

    printf("New user online: \"%s\"\n", sbon.username);
    return 0;
}

/* Handle the broadcast message that was sent be another client */
static int handle_broad_msg(void)
{
    struct sbm_payload sbm = {0};
    if (recv_payload_sbm(client.sock, &sbm) < 0)
        return -1;

    printf("Received broadcast: \"%s\"\n", sbm.msg);
    return 0;
}

/* This is used to handle message from the server that were send from a
 * different client */
static int handle_client_msg(void)
{
    struct sdmm_payload sdmm = {0};
    if (recv_payload_sdmm(client.sock, &sdmm) < 0)
        return -1;

    printf("Received direct message\n");
    printf("Sender: %s\n", sdmm.sender);
    printf("Message: %s\n", sdmm.msg);
    return 0;
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

        case broad_logon:
            return handle_broad_logon();

        case broad_msg:
            return handle_broad_msg();

        case client_msg:
            return handle_client_msg();

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
    int ret = 0;

    ret = get_payload(client.sock, &head, &payload);
    if (ret < 0)
        return -1;

    assert(head.task_id == server_command);
    ret = handle_scmd(payload);

    free(payload);
    return ret;
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

/* Return true if the "cmd" is asking for help, otherwise return false */
static bool is_help_cmd(char *cmd)
{
    const char *start = get_first_non_space(cmd);
    if (start == NULL)
        return false;

    if (strncmp(start, "help", 4) == 0)
        return true;

    if (strncmp(start, "?", 1) == 0)
        return true;

    return false;
}

/* Print the help for the client, don't need to talk to the server */
static void cmd_help(void)
{
    printf("All commands: \n");
    printf("  message <user> <message>\n");
    printf("  broadcase <message>\n");
    printf("  whoelse\n");
    printf("  whoelsesince <time>\n");
    printf("  block <user>\n");
    printf("  unblock <user>\n");
    printf("  logout\n");
    printf("  startprivate <user>\n");
    printf("  private <user> <message>\n");
    printf("  stopprivate <user>\n");
}

/* Used for when the client wants to block a user */
static int cmd_block(UNUSED struct scmd_payload *scmd)
{
    struct sbu_payload sbu = {0};
    if (recv_payload_sbu(client.sock, &sbu) < 0)
        return -1;

    switch (sbu.code) {
        case user_blocked:
            printf("User is already blocked!\n");
            return 0;

        case bad_uname:
            printf("Invalid username!\n");
            return 0;

        case task_success:
            printf("User successfully blocked!\n");
            return 0;

        case dup_error:
            printf("You can't blocker yourself\n");
            return 0;

        default:
            panic("Unkown sbu_payload, received: \"%s\"(%d)\n",
                code_to_str(sbu.code), sbu.code
            );

    }
}

/* Used for when the client sends to whoelse command to the server */
static int cmd_whoelse(struct scmd_payload *scmd)
{
    struct sw_payload sw = {0};
    int ret;

    uint64_t num_users = scmd->extra;

    if (num_users == 1)
        printf("There is 1 user online\n");
    else
        printf("There are %ld users online\n", num_users);

    uint64_t i;
    for (i = 0; i < num_users; i++) {
        ret = recv_payload_sw(client.sock, &sw);
        if (ret < 0)
            return -1;

        printf("%ld. \"%s\"\n", i+1, sw.username);
    }

    return 0;
}

/* Used for when the client sends to whoelsesince command to the server */
static int cmd_whoelsesince(struct scmd_payload *scmd)
{
    struct sws_payload sws = {0};
    int ret;

    uint64_t num_users = scmd->extra;

    if (num_users == 1)
        printf("There is 1 recently logged in user\n");
    else
        printf("There are %ld recently logged in users\n", num_users);

    uint64_t i;
    for (i = 0; i < num_users; i++) {
        ret = recv_payload_sws(client.sock, &sws);
        if (ret < 0)
            return -1;

        printf("%ld. \"%s\"\n", i+1, sws.username);
    }

    return 0;
}

/* Used for when the client sends the "broadcast" command to the server */
static int cmd_broadcast(UNUSED struct scmd_payload *scmd)
{
    // Client doesn't get response form server, therefore we pass
    return 0;
}

/* This is used when this client sends a message (via the server) to
 * another client. */
static int cmd_message(UNUSED struct scmd_payload *scmd)
{
    struct sdmr_payload sdmr = {0};
    if (recv_payload_sdmr(client.sock, &sdmr) < 0)
        return -1;

    switch (sdmr.code) {
        case task_success:
            printf("Message sent successfully\n");
            return 0;

        case msg_stored:
            printf("User off line, message stored\n");
            return 0;

        case user_blocked:
            printf("You have been blocked by receiver, message dropped\n");
            return 0;

        case dup_error:
            printf("You can't send a message to yourself, message dropped\n");
            return 0;

        case bad_uname:
            printf("You entered an invalid username!\n");
            return 0;

        default:
            panic("Received unknown response: \"%s\"(%d)\n",
                code_to_str(sdmr.code),
                sdmr.code
            );
    }
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
    if (is_help_cmd(cmd) == true) {
        cmd_help();
        return 0;
    }

    struct scmd_payload scmd = {0};

    if (send_payload_ccmd(client.sock, cmd) < 0) {
        return -1;
    }

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
