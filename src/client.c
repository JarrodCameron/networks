/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:35               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clogin.h"
#include "header.h"
#include "ptop.h"
#include "util.h"
#include "list.h"

#define DEFAULT_HINTS (struct addrinfo) { \
    .ai_family   = AF_INET,     /* ipv4 only */         \
    .ai_socktype = SOCK_STREAM, /* Use TCP */           \
    .ai_flags    = AI_PASSIVE,  /* Fill in sin_addr */  \
}

struct {
    struct sockaddr_in sockaddr;
    int sock;   /* For the server */
    bool is_logged_out;
    struct list *ptops;

    bool ptop_port_set;         /* If we have a port or not yet */
    unsigned short ptop_port;   /* Port used for peer to peer comms */

    int ptop_sock;  /* Peer to peer socket */

    char my_name[MAX_UNAME];
} client = {0};

/* Helper functions */
static int cmd_message(UNUSED struct scmd_payload *scmd);
static int cmd_unblock(UNUSED struct scmd_payload *scmd);
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
static int get_max_fd(fd_set *read_set);
static void fill_fd_set(fd_set *read_set);
static void cmd_help(void);
static int cmd_whoelse(struct scmd_payload *scmd);
static int set_ptop_sock(int server_sock);
static int cmd_whoelsesince(struct scmd_payload *scmd);
static int cmd_broadcast(UNUSED struct scmd_payload *scmd);
static int cmd_block(struct scmd_payload *scmd);
static const char *get_first_non_space(const char *line);
static int deploy_command(char cmd[MAX_COMMAND]);
static void run_client (void);
static int cmd_logout(struct scmd_payload *scmd);
static int host_to_sockaddr(const char *hostname, struct sockaddr_in *addr);
static int handle_broad_logoff(void);

/* The name of each command and the respective handle */
static struct {
    const char *name;
    int (*handle)(struct scmd_payload *);
} commands[] = {
    // WARNING: Do not change the order!!!
    {.name = "message",      .handle = cmd_message},
    {.name = "broadcast",    .handle = cmd_broadcast},
    {.name = "whoelsesince", .handle = cmd_whoelsesince},
    {.name = "whoelse",      .handle = cmd_whoelse},
    {.name = "block",        .handle = cmd_block},
    {.name = "unblock",      .handle = cmd_unblock},
    {.name = "logout",       .handle = cmd_logout},
};

/* Print usage and exit */
static void usage (void)
{
    fprintf (stderr, "Usage: ./client <host_name> <server_port>\n");
    exit (1);
}

/* Convert the hostname to an IPv4 address and return it in "addr", return -1
 * on error, otherwise return 0 */
static int host_to_sockaddr(const char *hostname, struct sockaddr_in *addr)
{
    struct addrinfo *res = NULL;
    struct addrinfo hints = DEFAULT_HINTS;

    int status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status < 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(status));
        return -1;
    }

    *addr = *(struct sockaddr_in *) res->ai_addr;
    freeaddrinfo(res);
    return 0;
}

/* Initialise the arguments, return -1 on error */
static int init_args (const char *ip_addr, const char *port)
{
    unsigned short dummy;
    struct sockaddr_in server_address = {0};

    if (host_to_sockaddr(ip_addr, &server_address) < 0)
        return -1;

    sscanf(port, "%hu", &dummy);
    if (dummy < 1024)
        return -1;
    server_address.sin_port = htons(dummy);

    server_address.sin_family = AF_INET;

    client.sockaddr = server_address;

    client.ptop_port_set = false;

    client.ptops = list_init();
    if (client.ptops == NULL)
        return -1;

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

    if (set_ptop_sock(sock) < 0) {
        close(sock);
        return -1;
    }

    if (conn_to_server(sock) < 0) {
        close (sock);
        close (client.ptop_sock);
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

int client_get_server_sock(void)
{
    return client.sock;
}

int client_get_ptop_sock(void)
{
    return client.ptop_sock;
}

struct list *client_get_ptops(void)
{
    return client.ptops;
}

void client_set_uname(const char name[MAX_UNAME])
{
    zero_out(client.my_name, MAX_UNAME);
    strncpy(client.my_name, name, MAX_UNAME);
}

char *client_get_uname(void)
{
    return safe_strndup(client.my_name, MAX_UNAME);
}

/* Return the number of items in the back log, if there is an error -1 is
 * returned */
static int get_backlog_len(void)
{
    struct scmd_payload scmd = {0};
    if (recv_payload_scmd(client.sock, &scmd) < 0)
        return -1;
    assert(scmd.code == backlog_msg);
    return scmd.extra;
}

/* Ask the server for a back log of messages if they have any */
static int handle_backlog(void)
{
    int backlog_len = get_backlog_len();
    if (backlog_len < 0)
        return -1;
    else if (backlog_len == 0)
        return 0;

    printf("You have received messages while you were offline!\n");

    struct sdmm_payload sdmm = {0};

    for (int i = 0; i < backlog_len; i++) {
        if (recv_payload_sdmm(client.sock, &sdmm) < 0)
            return -1;

        printf("Message id: %d\n", i+1);
        printf("  Sender: %s\n", sdmm.sender);
        printf("  Message: %s\n", sdmm.msg);
    }

    return 0;
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
    printf("  Sender: %s\n", sdmm.sender);
    printf("  Message: %s\n", sdmm.msg);
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

        case broad_logoff:
            return handle_broad_logoff();

        default:
            panic("Received unknown server command: \"%s\"(%d)\n",
                code_to_str(scmd->code), scmd->code
            );
    }
}

/* When a different client logs off their name is broadcasted, this is the
 * handler for when that happens */
static int handle_broad_logoff(void)
{
    struct sbof_payload sbof = {0};
    if (recv_payload_sbof(client.sock, &sbof) < 0)
        return -1;

    printf("User logged out: \"%s\"\n", sbof.name);
    return 0;
}

/* This is called when the main loop receives a message from the socket
 * and needs to be handled */
static int socket_read_handle(void)
{
    struct header head = {0};
    void *payload = NULL;
    int ret = 0;

    ret = get_payload(client.sock, &head, &payload);
    if (ret < 0)
        return -1;

    switch (head.task_id) {
        case server_command:
            ret = handle_scmd(payload);
            break;

        default:
            panic("Received bad header: \"%s\"(%d)\n",
                id_to_str(head.task_id),
                head.task_id
            );
    }

    free(payload);
    return ret;
}

/* This is called when the main loop receives a message from standard input
 * and needs to be handled */
static int stdin_read_handle(void)
{
    bool is_error = false;
    char cmd[MAX_COMMAND] = {0};

    int ret = read(0, cmd, MAX_COMMAND-1);

    if (ret <= 1)
        return 0;

    if (cmd[ret-1] == '\n' || cmd[ret-1] == '\r')
        cmd[ret-1] = '\0';


    if (ptop_is_cmd(cmd) == true) {
        if (ptop_handle_cmd(cmd) < 0)
            is_error = true;
    } else if (deploy_command(cmd) < 0)
        is_error = true;

    if (is_error == true) {
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

/* Used for when this user wants to unblock another user */
static int cmd_unblock(UNUSED struct scmd_payload *scmd)
{
    struct suu_payload suu = {0};
    if (recv_payload_suu(client.sock, &suu) < 0)
        return -1;

    switch (suu.code) {
        case user_unblocked:
            printf("The user was not blocked!\n");
            return 0;

        case task_success:
            printf("User successfully unblocked!\n");
            return 0;

        case bad_uname:
            printf("User does not exist!\n");
            return 0;

        case dup_error:
            printf("You can't unblock yourself\n");
            return 0;

        default:
            panic("Unkown suu_payload, received: \"%s\"(%d)\n",
                code_to_str(suu.code), suu.code
            );

    }
}

/* Set up the peer to peer socket after the server socket is done */
static int set_ptop_sock(int server_sock)
{
    int ret, ptop_sock;
    struct sockaddr_in temp_addr = {0};
    socklen_t temp_len = sizeof(temp_addr);

    ptop_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ptop_sock < 0)
        return -1;

    ret = getsockname(server_sock, (struct sockaddr *) &temp_addr, &temp_len);
    if (ret < 0) {
        close(ptop_sock);
        return -1;
    }

    temp_addr.sin_family = AF_INET;
    temp_addr.sin_port = htons(temp_addr.sin_port);
    temp_addr.sin_addr = temp_addr.sin_addr;

    ret = bind(ptop_sock, (struct sockaddr *) &temp_addr, temp_len);
    if (ret < 0) {
        close(ptop_sock);
        return -1;
    }

    ret = listen(ptop_sock, CLIENT_BACKLOG);
    if (ret < 0) {
        close(ptop_sock);
        return -1;
    }

    client.ptop_sock = ptop_sock;
    return 0;
}

/* This is called when the client wants to logout of their session */
static int cmd_logout(UNUSED struct scmd_payload *scmd)
{
    client.is_logged_out = true;
    return ptop_stop_all();
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
static int cmd_broadcast(struct scmd_payload *scmd)
{
    if (scmd->extra == 0) {
        printf("Broadcast was successful!\n");
    } else {
        printf("Broadcast was blocked by %ld online users\n", scmd->extra);
    }
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

/* Select a varaible number of sockets at the same time, return -1 on error
 * otherwise zero is returned */
static int multi_select(fd_set *read_set)
{
    fill_fd_set(read_set);
    int nfds = get_max_fd(read_set) + 1;
    return select(nfds, read_set, NULL, NULL, NULL);
}

/* Fill the fd set will all fd's that need to be listened to */
static void fill_fd_set(fd_set *read_set)
{
    FD_ZERO(read_set);
    FD_SET(client.sock, read_set);
    FD_SET(0 /* stdin */, read_set);
    FD_SET(client.ptop_sock, read_set);
    ptop_fill_fd_set(read_set);
}

/* Return the large file descriptor in the set of file descritpors */
static int get_max_fd(fd_set *read_set)
{
    int max_fd = -1;
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, read_set))
            max_fd = MAX(max_fd, i);
    }
    return max_fd;
}

/* Run the client, this is where the communication to the server takes place.
 * This is logs the client in to the server */
static void run_client (void)
{
    if (attempt_login(client.sock) < 0)
        return;

    client.is_logged_out = false;

    if (handle_backlog() < 0)
        return;

    while (1) {

        fd_set read_set;

        printf("> ");
        fflush(stdout);

        if (multi_select(&read_set) < 0) {
            printf("Failed to receive input from server or client\n");
            return;
        }

        if (FD_ISSET(client.sock, &read_set)) {
            FD_CLR(client.sock, &read_set);
            if (socket_read_handle() < 0) {
                printf("Failed to handle socket payload\n");
                return;
            }
        }

        if (FD_ISSET(0 /* stdin */, &read_set)) {
            FD_CLR(0, &read_set);
            if (stdin_read_handle() < 0) {
                printf("Failed to handle stdin payload\n");
                return;
            }
        }

        if (FD_ISSET(client.ptop_sock, &read_set)) {
            FD_CLR(client.ptop_sock, &read_set);
            if (ptop_accept() < 0) {
                printf("Failed peer to peer communication\n");
                return;
            }
        }

        if (ptop_handle_read_set(&read_set) < 0) {
            printf("Failed peer to peer communication\n");
            return;
        }

        if (client.is_logged_out == true)
            break;
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
