/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   19/11/19  9:13               *
 *                                         *
 *******************************************/

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "ptop.h"
#include "util.h"
#include "header.h"

/* Helper functions */
static const char *get_first_non_space(const char *line);
static int deploy_ptop_cmd(char *cmd, struct tokens *toks);
static bool already_started(const char *name);
static int query_ptop(char *cmd, struct tokens *toks, struct ptop **ptop);
static int add_client_ptop(struct ptop *ptop);
static int init_ptop_conn(struct ptop *ptop);
static void remove_ptop(struct ptop *ptop);
static struct ptop *ptop_init_with_name(int sock, char name[MAX_UNAME]);
static int add_ptop_session(int sock, char name[MAX_UNAME]);
static int ssp_handle_code(struct ssp_payload ssp);
static int ptop_init_handshake(int sock);
static void ptop_free(struct ptop *ptop);
static void started_iter(void *item, void *arg);
static int ptop_cmp(void *p1, void *p2);
static struct ptop *ptop_init(char *uname, unsigned short port, struct in_addr addr);
static int cmd_stopprivate(char *safe_cmd, struct tokens *toks);
static int cmd_private(char *safe_cmd, struct tokens *toks);
static int fill_ptop(char *uname, struct ptop **ptop);
static int init_conn_to_server(char *cmd);
static int cmd_startprivate(char *safe_cmd, struct tokens *toks);
static int deploy_sock_payload(struct tokens *toks);
static int sock_startprivate(struct tokens *toks);

/* Similar to the connection in connection.[ch] */
struct ptop {
    char peer_name[MAX_UNAME];  /* The client we are talking to */

    /* Retreived from server */
    unsigned short peer_port;   /* The port number of the peer */
    struct in_addr peer_addr;   /* IP address of the peer */

    int sock;                   /* For sends/recv's */
};

/* All of the peer to peer commands, and respective functions that should
 * be called when invoked */
static struct {
    /* The name of the command */
    const char *name;

    /* Handle for commands via stdin */
    int (*cmd_handle)(char *safe_cmd, struct tokens *);

    /* Handle socket payloads */
    int (*sock_handle)(struct tokens *);
} commands[] = {
    {
        .name = "startprivate",
        .cmd_handle = cmd_startprivate,
        .sock_handle = sock_startprivate,
    },
    {
        .name = "private",
        .cmd_handle = cmd_private,
        .sock_handle = NULL,
    },
    {
        .name = "stopprivate",
        .cmd_handle = cmd_stopprivate,
        .sock_handle = NULL,
    },
};

bool ptop_is_cmd(char cmd[MAX_MSG_LENGTH])
{
    const char *start = get_first_non_space(cmd);
    if (start == NULL)
        return false;

    unsigned int i;
    for (i = 0; i < ARRSIZE(commands); i++) {
        const char *name = commands[i].name;

        if (strncmp(start, name, strlen(name)) == 0)
            return true;
    }
    return false;
}

int ptop_handle_cmd(char cmd[MAX_MSG_LENGTH])
{
    struct tokens *toks = NULL;

    char *safe_cmd = safe_strndup(cmd, MAX_MSG_LENGTH);
    if (safe_cmd == NULL)
        return -1;

    if (tokenise(cmd, &toks) < 0) {
        free(safe_cmd);
        return -1;
    }

    int ret = deploy_ptop_cmd(safe_cmd, toks);
    tokens_free(toks);
    free(safe_cmd);
    return ret;

}

int ptop_handle_payload(void *payload)
{
    struct pcmd_payload *pcmd = payload;
    struct tokens *toks = NULL;

    if (tokenise(pcmd->cmd, &toks) < 0)
        return -1;

    int ret = deploy_sock_payload(toks);

    tokens_free(toks);
    return ret;
}

int ptop_accept(void)
{
    struct phs_payload phs = {0};
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);

    int accept_sock = client_get_ptop_sock();

    int sock = accept(accept_sock, (struct sockaddr *) &addr, &len);
    if (sock < 0)
        return -1;

    if (recv_payload_phs(sock, &phs) < 0) {
        close(sock);
        return -1;
    }

    if (add_ptop_session(sock, phs.name) < 0) {
        close(sock);
        return -1;
    }

    printf("Started private connection with: \"%s\"\n", phs.name);

    return 0;
}

/* Add a peer to peer connection to the list of ptop connections */
static int add_ptop_session(int sock_to_add, char name[MAX_UNAME])
{
    struct ptop *ptop = ptop_init_with_name(sock_to_add, name);
    if (ptop == NULL)
        return -1;

    struct list *ptops = client_get_ptops();

    if (list_add(ptops, ptop) < 0) {
        ptop_free(ptop);
        return -1;
    }

    return 0;
}

/* Given the tokens send this to the appropriate function */
static int deploy_sock_payload(struct tokens *toks)
{
    unsigned int i;
    for (i = 0; i < ARRSIZE(commands); i++) {
        if (strcmp(toks->toks[0], commands[i].name) == 0)
            return commands[i].sock_handle(toks);
    }
    panic("Received bad socket payload: \"%s\"\n", toks->toks[0]);
}

/* Client received socket message from a peer to start a private conn */
static int sock_startprivate(UNUSED struct tokens *toks)
{
    panic("This function should never be called\n");
}

/* Initialise the ptop struct with the name and socket */
static struct ptop *ptop_init_with_name(int sock, char name[MAX_UNAME])
{
    struct ptop *ptop = malloc(sizeof(struct ptop));
    if (ptop == NULL)
        return NULL;

    *ptop = (struct ptop) {0};

    ptop->sock = sock;
    strncpy(ptop->peer_name, name, MAX_UNAME);
    return ptop;
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

/* Deploy the commannd to the network */
static int deploy_ptop_cmd(char *cmd, struct tokens *toks)
{
    unsigned int i;
    for (i = 0; i < ARRSIZE(commands); i++) {
        const char *name = commands[i].name;
        int (*handle)(char *, struct tokens *) = commands[i].cmd_handle;

        if (strncmp(toks->toks[0], name, strlen(name)) == 0)
            return handle(cmd, toks);
    }
    panic("Receive unknown peer to peer command: \"%s\"\n", toks->toks[0]);
}

/* This is used to handle the "startprivate" command by the client */
static int cmd_startprivate(char *safe_cmd, struct tokens *toks)
{
    struct ptop *ptop = NULL;
    if (already_started(toks->toks[1]) == true) {
        printf("Already in session with: \"%s\"\n", toks->toks[1]);
        return 0;
    }

    if (query_ptop(safe_cmd, toks, &ptop) < 0)
        return -1;

    if (ptop == NULL)
        return 0;

    if (add_client_ptop(ptop) < 0) {
        ptop_free(ptop);
        return -1;
    }

    if (init_ptop_conn(ptop) < 0) {
        remove_ptop(ptop);
        ptop_free(ptop);
        return -1;
    }

    printf("Successfully started peer to peer connection!\n");
    return 0;
}

/* This is used to handle the "private" command by the client */
static int cmd_private(char *safe_cmd, struct tokens *toks)
{
    panic("The cmd_private command\n");
    // todo: The cmd_private command
    (void) toks;
    (void) safe_cmd;
    return -1;
}

/* This is used to handle the "stopprivate" command by the client */
static int cmd_stopprivate(char *safe_cmd, struct tokens *toks)
{
    panic("The cmd_stopprivate command\n");
    // todo: The cmd_stopprivate command
    (void) toks;
    (void) safe_cmd;
    return -1;
}

/* If the client is already in a session with this user then true is returned,
 * otherwise false is returned */
static bool already_started(const char *name)
{
    struct list *ptops = client_get_ptops();

    bool started = false;

    struct tuple t = {
        .items[0] = &started,
        .items[1] = (void *) name,
    };

    list_traverse(ptops, started_iter, &t);
    return started;
}

/* Iterator over the list of ptops, arg->items[0] = *started. "started" is
 * set to true if the name matches a ptop */
static void started_iter(void *item, void *arg)
{
    struct ptop *ptop = item;
    struct tuple *t = arg;
    bool *started = t->items[0];
    char *name = t->items[1];

    if (*started == true)
        return;

    *started = (strncmp(ptop->peer_name, name, MAX_UNAME-1) == 0);
}

/* Ask the server for the IP address and the port number of the desired client.
 * If there is a communication error -1 is returned otherwise 0 is returned.
 * If server doesn't give the details (e.g. user blocked) then *prop is
 * untouched */
static int query_ptop(char *cmd, struct tokens *toks, struct ptop **ptop)
{
    if (init_conn_to_server(cmd) < 0)
        return -1;

    if (fill_ptop(toks->toks[1], ptop) < 0)
        return -1;

    return 0;
}

/* Simple add the ptop to the list of ptops in the client. Return value is
 * the result of the list_add() */
static int add_client_ptop(struct ptop *ptop)
{
    assert(ptop != NULL);
    struct list *ptops = client_get_ptops();
    return list_add(ptops, ptop);
}

/* Initialise the connection to the ptop, return -1 on error, otherwise
 * zero is returned */
static int init_ptop_conn(struct ptop *ptop)
{

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_in addr = {
        .sin_addr = ptop->peer_addr,
        .sin_port = htons(ptop->peer_port),
        .sin_family = AF_INET,
    };

    int ret = connect(
        sock,
        (struct sockaddr *) &addr,
        sizeof(struct sockaddr_in)
    );
    if (ret < 0) {
        close(sock);
        return -1;
    }

    if (ptop_init_handshake(sock) < 0) {
        close(sock);
        return -1;
    }

    ptop->sock = sock;
    return 0;
}

/* The the handshake to sync names between the two users */
static int ptop_init_handshake(int sock)
{
    char *name = client_get_uname();
    if (name == NULL)
        return -1;
    int ret = send_payload_phs(sock, name);
    free(name);
    return ret;
}

/* Remove the ptop to the list of ptops in the client */
static void remove_ptop(struct ptop *ptop)
{
    struct list *ptops = client_get_ptops();
    list_rm(ptops, ptop, ptop_cmp);
}

/* Return the different between two ptops. The easiest way to tell the
 * difference is strncmp on the names */
static int ptop_cmp(void *p1, void *p2)
{
    struct ptop *ptop1 = p1;
    struct ptop *ptop2 = p2;
    return strncmp(ptop1->peer_name, ptop2->peer_name, MAX_UNAME);
}

/* Simple free all the alloced members of the ptop */
static void ptop_free(struct ptop *ptop)
{
    free(ptop);
}

/* Initialise the connection to the server to get the goods, -1 is returned
 * on error, otherwise 0 is returned */
static int init_conn_to_server(char *cmd)
{
    int sock = client_get_server_sock();
    if (send_payload_ccmd(sock, cmd) < 0)
        return -1;

    struct scmd_payload scmd = {0};
    if (recv_payload_scmd(sock, &scmd) < 0)
        return -1;

    assert(scmd.code == task_ready);
    return 0;
}

/* Query the server for the contents of the ptop and fill it up, -1 is
 * returned for comms error otherwise zero is returned, if denied access
 * then *ptop is left alone */
static int fill_ptop(char *uname, struct ptop **ptop)
{
    struct ssp_payload ssp = {0};

    int sock = client_get_server_sock();
    if (recv_payload_ssp(sock, &ssp) < 0)
        return -1;

    if (ssp_handle_code(ssp) < 0)
        return 0;

    struct ptop *ptop_ret = ptop_init(uname, ssp.port, ssp.addr);
    if (ptop_ret == NULL)
        return -1;

    *ptop = ptop_ret;
    return 0;
}

/* Return 0 if this ssp_payload is valid, otherwise return -1. NOTE: -1
 * doesn't mean it is invalid by means not worth using */
static int ssp_handle_code(struct ssp_payload ssp)
{
    switch (ssp.code) {
        case bad_uname:
            printf("Invalid username, user does not exist!\n");
            return -1;

        case dup_error:
            printf("You can not private message yourself!\n");
            return -1;

        case user_blocked:
            printf("You have been blocked by the user!\n");
            return -1;

        case user_offline:
            printf("The user is currently offline!\n");
            return -1;

        case task_success:
            return 0;

        default:
            panic("Unknown ssp_handle code: \"%s\"\n",
                code_to_str(ssp.code), ssp.code
            );
    }
}

/* Create and return a fresh ptop* */
static struct ptop *ptop_init(char *uname, unsigned short port, struct in_addr addr)
{
    struct ptop *ret = malloc(sizeof(struct ptop));
    if (ret == NULL)
        return NULL;

    *ret = (struct ptop) {0};

    strncpy(ret->peer_name, uname, MAX_UNAME-1);
    ret->peer_port = port;
    ret->peer_addr = addr;
    return ret;
}
