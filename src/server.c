/*******************************************;
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:33               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include "connection.h"
#include "logger.h"
#include "slogin.h"
#include "util.h"

/* For returning a service function pointer */
typedef int (*service_handle)(int sock, struct tokens *, struct user *);
typedef void (*log_handle)(struct tokens *, const char *);

static struct {

    /* User args */
    time_t block_duration;      /* How long to block a user */
    int port;                   /* Port to listen to connections */
    int timeout;                /* How long until a user gets kicked */

    int listen_sock;            /* The socket to listen on (i.e. the fd) */

    struct list *users;         /* List of all valid users */

    struct list *connections;   /* All connections to clients */

    time_t time_started;        /* The exact time the server started */
} server = {0};

/* Helper functions */
static void log_command(struct tokens *tokes, struct user *);
static void message_logger(struct tokens *toks, const char *user_name);
static void broadcast_logger(struct tokens *toks, const char *user_name);
static void whoelsesince_logger(struct tokens *toks, const char *user_name);
static void whoelse_logger(struct tokens *toks, const char *user_name);
static void block_logger(struct tokens *toks, const char *user_name);
static void unblock_logger(struct tokens *toks, const char *user_name);
static void logout_logger(struct tokens *toks, const char *user_name);
static void startprivate_logger(struct tokens *toks, const char *user_name);
static int message_service(int sock, struct tokens *toks, struct user *user);
static int logout_service(int sock, struct tokens *toks, struct user *user);
static void usage (void);
static int unblock_service(int sock, struct tokens *toks, struct user *user);
static int init_users (void);
static int init_args (const char *port, const char *dur, const char *timeout);
static int init_server (void);
static int deploy_full_ssp(int sock, struct connection *conn);
static int send_default_ssp(int sock, enum status_code code);
static int startprivate_service(int sock, struct tokens *toks, struct user *);
static void free_users (void);
static int block_service(int sock, struct tokens *toks, struct user *user);
static int dispatch_event (struct connection *conn);
static void client_command_handler(int sock, struct user *user);
static int set_timeout(int sock);
static int timeout_user(int sock, struct user *user);
static int whoelse_service (int sock, struct tokens *toks, struct user *user);
static int whoelsesince_service (int sock, struct tokens *, struct user *user);
static int broadcast_service (int sock, struct tokens *, struct user *user);
static int ptr_cmp (void *a, void *b);
static enum status_code deploy_message(struct user *r, struct user *s, const char *msg);

/* The name of each command and the respective handle */
struct {
    const char *name;
    service_handle service;
    log_handle logger;
} command_services[] = {
    // WARNING: Do not change the order!!!
    {
        .name = "message",
        .service = message_service,
        .logger = message_logger
    },
    {
        .name = "broadcast",
        .service = broadcast_service,
        .logger = broadcast_logger
    },
    {
        .name = "whoelsesince",
        .service = whoelsesince_service,
        .logger = whoelsesince_logger
    },
    {
        .name = "whoelse",
        .service = whoelse_service,
        .logger = whoelse_logger
    },
    {
        .name = "block",
        .service = block_service,
        .logger = block_logger
    },
    {
        .name = "unblock",
        .service = unblock_service,
        .logger = unblock_logger
    },
    {
        .name = "logout",
        .service = logout_service,
        .logger = logout_logger
    },
    {
        .name = "startprivate",
        .service = startprivate_service,
        .logger = startprivate_logger
    },
};

/* Logger for the "message" command */
static void message_logger(struct tokens *toks, const char *user_name)
{
    logs("Message: \"%s\" -> \"%s\"\n", user_name, toks->toks[1]);
}

/* Logger for the "broadcast" command */
static void broadcast_logger(UNUSED struct tokens *toks, const char *user_name)
{
    logs("Broadcast: \"%s\"\n", user_name);
}

/* Logger for the "whoelsesince" command */
static void whoelsesince_logger(struct tokens *toks, const char *user_name)
{
    logs("Whoelsesince: \"%s\" t=%s\n", user_name, toks->toks[1]);
}

/* Logger for the "whoelse" command */
static void whoelse_logger(UNUSED struct tokens *toks, const char *user_name)
{
    logs("Whoelse: \"%s\"\n", user_name);
}

/* Logger for the "block" command */
static void block_logger(struct tokens *toks, const char *user_name)
{
    logs("Block: \"%s\" -> \"%s\"\n", user_name, toks->toks[1]);
}

/* Logger for the "unblock" command */
static void unblock_logger(struct tokens *toks, const char *user_name)
{
    logs("Unblock: \"%s\" -> \"%s\"\n", user_name, toks->toks[1]);
}

/* Logger for the "logout" command */
static void logout_logger(UNUSED struct tokens *toks, const char *user_name)
{
    logs("Logout: \"%s\"\n", user_name);
}

/* Logger for the "startprivate" command */
static void startprivate_logger(struct tokens *toks, const char *user_name)
{
    logs("Startprivate: \"%s\" -> \"%s\"\n", user_name, toks->toks[1]);
}

/* Print usage and exit */
static void usage (void)
{
    fprintf(stderr,
        "Usage: ./server <server_port> <block_duration> <timeout>\n"
    );
    exit(1);
}

/* This is where the new thread starts.
 * This will do the username and password authentication for the server.
 * If the user can't be authenicated the thread is killed */
static void *thread_landing (void *arg)
{
    struct connection *conn = arg;
    int sock = conn_get_sock(conn);
    struct user *curr_user;

    // Send the ACK, unblock client
    if (send_payload_sic(sock, init_success) < 0) {
        conn_free(conn);
        return NULL;
    }

    curr_user = auth_user(sock, server.users);
    if (curr_user == NULL) {
        conn_free(conn);
        return NULL;
    }
    conn_set_user(conn, curr_user);

    if (conn_broad_log_on(server.connections, curr_user) < 0) {
        conn_free(conn);
        return NULL;
    }

    if (list_add(server.connections, conn) < 0) {
        conn_free(conn);
        return NULL;
    }

    set_timeout(sock);

    client_command_handler(sock, curr_user);

    list_rm(server.connections, conn, ptr_cmp);
    conn_free(conn);

    logs("Connection closed... killing thread\n");
    return NULL;
}

/* Set the timeout time for when the socket is receiving from the client */
static int set_timeout(int sock)
{
    struct timeval tv = sec_to_tv(server.timeout);
    int ret = setsockopt(
        sock,
        SOL_SOCKET,     // Needed to edit socket api
        SO_RCVTIMEO,    // Timeout on recv
        (char *) &tv,
        sizeof(struct timeval)
    );

    // This should never fail
    assert(ret >= 0);

    return ret;
}

/* This is called when the user wants to log out */
static int logout_service(int sock, UNUSED struct tokens *t, struct user *user)
{
    user_log_off(user);

    if (send_payload_scmd(sock, task_ready, 0 /* ignored */) < 0)
        return -1;

    conn_broad_log_off(server.connections, user);
    return 0;
}

/* This is called when the user wants to start a private connections and
 * needs details about the respective user. This includes port number
 * and IPv4 address */
static int startprivate_service(int sock, struct tokens *toks, struct user *user)
{
    if (send_payload_scmd(sock, task_ready, 0 /* ignored */) < 0)
        return -1;

    struct user *receiver = user_get_by_name(server.users, toks->toks[1]);
    if (receiver == NULL)
        return send_default_ssp(sock, bad_uname);

    if (user_equal(receiver, user) == true)
        return send_default_ssp(sock, dup_error);

    if (user_on_blocklist(receiver, user) == true)
        return send_default_ssp(sock, user_blocked);

    struct connection *conn = NULL;
    if (conn_get_by_user(server.connections, receiver, &conn) < 0)
        return -1;

    if (conn == NULL)
        return send_default_ssp(sock, user_offline);

    return deploy_full_ssp(sock, conn);
}

/* Deploy the full ssp payload to the receiver specified by the sock,
 * not the receiver specified by the conn */
static int deploy_full_ssp(int sock, struct connection *conn)
{
    unsigned short port = conn_get_port(conn);
    struct in_addr addr = conn_get_in_addr(conn);
    return send_payload_ssp(sock, task_success, port, addr);
}

/* Return a ssp_payload where the code is specified but the port and addr
 * fields are filled with trash */
static int send_default_ssp(int sock, enum status_code code)
{
    int port = 0;
    struct in_addr addr = {0};
    return send_payload_ssp(sock, code, port, addr);
}

/* The current user wants to block user toks->toks[1] */
static int unblock_service(int sock, struct tokens *toks, struct user *user)
{
    assert(toks->toks[1] != NULL);

    char *safe_name = safe_strndup(toks->toks[1], MAX_UNAME);
    if (safe_name == NULL)
        return -1;

    if (send_payload_scmd(sock, task_ready, 0 /* ignored */) < 0) {
        free(safe_name);
        return -1;
    }

    enum status_code code = user_unblock(server.users, user, safe_name);
    if (code == server_error) {
        free(safe_name);
        return -1;
    }

    free(safe_name);
    return send_payload_suu(sock, code);
}

/* The current user wants to block user toks->toks[1] */
static int block_service(int sock, struct tokens *toks, struct user *user)
{
    assert(toks->toks[1] != NULL);

    char *safe_name = safe_strndup(toks->toks[1], MAX_UNAME);
    if (safe_name == NULL)
        return -1;

    if (send_payload_scmd(sock, task_ready, 0 /* ignored */) < 0) {
        free(safe_name);
        return -1;
    }

    enum status_code code = user_block(server.users, user, safe_name);
    if (code == server_error) {
        free(safe_name);
        return -1;
    }

    free(safe_name);
    return send_payload_sbu(sock, code);
}

/* Handle sending the client the result of the whoelse command */
static int whoelse_service
(
    int sock,
    UNUSED struct tokens *toks,
    struct user *user
)
{
    struct list *whoelse_list = user_whoelse(server.users, user);
    if (whoelse_list == NULL)
        return -1;

    int len = list_len(whoelse_list);
    int ret = send_payload_scmd(sock, task_ready, len);
    if (ret < 0) {
        list_free(whoelse_list, free);
        return -1;
    }

    while (list_is_empty(whoelse_list) == false) {
        char *name = list_pop(whoelse_list);
        if (send_payload_sw(sock, name) < 0) {
            list_free(whoelse_list, free);
            free(name);
            return -1;
        }
        free(name);
    }

    list_free(whoelse_list, free);

    return 0;
}

/* Handle sending the client the result of the whoelsesince command */
static int whoelsesince_service
(
    int sock,
    struct tokens *toks,
    struct user *user
)
{
    time_t off_time;
    sscanf(toks->toks[1], "%ld", &off_time);

    struct list *whoelse_list = user_whoelsesince(server.users, user, off_time);
    if (whoelse_list == NULL)
        return -1;

    int len = list_len(whoelse_list);
    int ret = send_payload_scmd(sock, task_ready, len);
    if (ret < 0) {
        list_free(whoelse_list, free);
        return -1;
    }

    while (list_is_empty(whoelse_list) == false) {
        char *name = list_pop(whoelse_list);
        if (send_payload_sws(sock, name) < 0) {
            list_free(whoelse_list, free);
            free(name);
            return -1;
        }
        free(name);
    }

    list_free(whoelse_list, free);

    return 0;
}

/* Used for the user to send another message to the user by the name
 * of "toks->toks[1]". The message is stored in "toks->toks[2]" */
static int message_service(int sock, struct tokens *toks, struct user *user)
{
    if (send_payload_scmd(sock, task_ready, 0 /* ignored */) < 0)
        return -1;

    if (user_uname_cmp(user, toks->toks[1]) == 0)
        return send_payload_sdmr(sock, dup_error);

    struct user *receiver = user_get_by_name(server.users, toks->toks[1]);
    if (receiver == NULL)
        return send_payload_sdmr(sock, bad_uname);

    if (user_on_blocklist(receiver, user) == true)
        return send_payload_sdmr(sock, user_blocked);

    char *safe_msg = malloc(MAX_MSG_LENGTH);
    if (safe_msg == NULL)
        return -1;
    zero_out(safe_msg, MAX_MSG_LENGTH);
    strncpy(safe_msg, toks->toks[2], MAX_MSG_LENGTH);

    enum status_code code = deploy_message(receiver, user, safe_msg);
    if (code == kill_me_now) {
        free(safe_msg);
        return -1;
    }

    free(safe_msg);
    return send_payload_sdmr(sock, code);
}

/* Do the actual sending of the message */
static enum status_code deploy_message
(
    struct user *receiver,
    struct user *sender,
    const char *msg
)
{
    struct connection *recv_conn;

    char *sender_name = user_get_uname(sender);
    if (sender_name == NULL)
        return kill_me_now;

    if (conn_get_by_user(server.connections, receiver, &recv_conn) < 0)
        return -1;

    if (recv_conn == NULL) {
        if (user_add_to_backlog(receiver, sender_name, msg) < 0)
            return kill_me_now;
        return msg_stored;
    }

    int sock = conn_get_sock(recv_conn);

    if (send_payload_scmd(sock, client_msg, 0 /* ignored */) < 0) {
        free(sender_name);
        return kill_me_now;
    }

    if (send_payload_sdmm(sock, sender_name, msg) < 0) {
        free(sender_name);
        return kill_me_now;
    }

    return task_success;
}

time_t server_uptime(void)
{
    return time(NULL) - server.time_started;
}

time_t server_block_dur(void)
{
    return server.block_duration;
}

/* Used to broadcast the message sent by the server to all other clients */
static int broadcast_service(int sock, struct tokens *toks, struct user *user)
{
    int num_blocked = conn_get_num_blocked(server.connections, user);
    if (send_payload_scmd(sock, task_ready, num_blocked) < 0)
        return -1;

    char *safe_msg = safe_strndup(toks->toks[1], MAX_MSG_LENGTH-1);
    int ret = conn_broad_msg(server.connections, user, safe_msg);
    free(safe_msg);
    return ret;
}

/* The client has given us a command which must be serviced, return a pointer
 * to the function that does the servicing */
static service_handle lookup_service(const char *cmd)
{
    unsigned int i;
    for (i = 0; i < ARRSIZE(command_services); i++) {
        const char *name = command_services[i].name;
        if (strncmp(cmd, name, strlen(name)-1) == 0) {
            return command_services[i].service;
        }
    }

    panic("The lookup service only expects valid commands!\n");
}

/* The client has send me a question and I shall answer it */
static int service_query
(
    int sock,
    struct user *user,
    struct header head,
    void *payload
)
{
    if (head.task_id != client_command) {
        panic(
            "Received bad query: \"%s\"(%d)\n",
            id_to_str(head.task_id),
            head.task_id
        );
    }

    struct ccmd_payload *ccmd = payload;

    struct tokens *toks = NULL;
    int ret = tokenise(ccmd->cmd, &toks);
    if (ret < 0) {
        return -1;
    }

    if (toks == NULL) {
        ret = send_payload_scmd(sock, bad_command, 0 /* ignored */);
        return (ret < 0) ? -1 : 0;
    }

    log_command(toks, user);

    service_handle service = lookup_service(toks->toks[0]);
    assert(service != NULL);

    ret = service(sock, toks, user);
    tokens_free(toks);
    return ret;
}

/* The client has just logged in and needs to receive their backlog of
 * messages */
static int handle_backlog(int sock, struct user *user)
{
    int backlog_len = user_get_backlog_len(user);
    if (send_payload_scmd(sock, backlog_msg, backlog_len) < 0)
        return -1;

    struct sdmm_payload *sdmm = NULL;
    for (int i = 0; i < backlog_len; i++) {
        sdmm = user_pop_backlog(user);
        assert(sdmm != NULL);
        if (send_payload_sdmm(sock, sdmm->sender, sdmm->msg) < 0) {
            free(sdmm);
            return -1;
        }
        free(sdmm);
    }
    return 0;
}

/* Log the command to the display */
static void log_command(struct tokens *toks, struct user *user)
{
    char *user_name = user_get_uname(user);
    if (user_name == NULL)
        return;

    unsigned int i;
    for (i = 0; i < ARRSIZE(command_services); i++) {
        const char *name = command_services[i].name;
        log_handle logger = command_services[i].logger;
        if (strcmp(toks->toks[0], name) == 0) {
            logger(toks, user_name);
            break;
        }
    }
    free(user_name);
}

/* This handles all commands sent from the client to the server for commands,
 * not for spawning connections */
static void client_command_handler(int sock, struct user *user)
{
    struct header head;
    void *payload;
    int ret;

    if (handle_backlog(sock, user) < 0)
        return;

    while (1) {
        ret = get_payload(sock, &head, &payload);

        if (ret == -EAGAIN || ret == -EWOULDBLOCK) {
            timeout_user(sock, user);
            return;
        }

        if (ret < 0)
            return;

        if (service_query(sock, user, head, payload) < 0) {
            free (payload);
            return;
        }

        free (payload);

        if (user_is_logged_on(user) == false)
            return;
    }
}

/* The user has received a timeout and needs to be logged out */
static int timeout_user(int sock, struct user *user)
{
    int ret = send_payload_scmd(sock, time_out, 0 /* ignored */);
    user_log_off(user);
    return ret;
}

/* Initialise the users list from the CRED_LIST file */
static int init_users (void)
{
    char uname[MAX_UNAME] = {0};
    char pword[MAX_PWORD] = {0};

    server.users = list_init();
    if (server.users == NULL)
        return -1;

    FILE *f = fopen(CRED_LIST, "r");

    while (fscanf(f, "%s %s", uname, pword) == 2) {

        struct user *user = user_init(uname, pword);

        if (user == NULL)
            goto init_users_fail;

        if (list_add(server.users, user) < 0)
            goto init_users_fail;

        zero_out(uname, MAX_UNAME);
        zero_out(pword, MAX_PWORD);
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

    sscanf(dur, "%ld", &(server.block_duration));

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

    server.connections = list_init();
    if (server.connections == NULL)
        return -1;

    if (server.listen_sock < 0) {
        list_free(server.connections, NULL);
        return -1;
    }

    ret = bind(
        server.listen_sock,
        (struct sockaddr *) &server_address,
        sizeof(server_address)
    );
    if (ret < 0) {
        close(server.listen_sock);
        list_free(server.connections, NULL);
        return -1;
    }

    ret = listen(
        server.listen_sock,
        SERVER_BACKLOG
    );
    if (ret < 0) {
        close(server.listen_sock);
        list_free(server.connections, NULL);
        return -1;
    }

    return 0;
}

/* Free the list of users from memory (memory leaks are bad) */
static void free_users (void)
{
    if (server.users == NULL)
        return;
    list_free(server.users, (void*) user_free);
}

static int ptr_cmp (void *a, void *b)
{
    return (a - b);
}

/* Probe handler list to dispatch even and spawn new thread */
static int dispatch_event (struct connection *conn)
{
    pthread_t *tid = malloc(sizeof(pthread_t));
    if (tid == NULL)
        return -1;

    conn_set_thread(conn, tid);

    if (pthread_create(tid, NULL, thread_landing, conn) != 0)
        return -1;

    return 0;
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

        logs("New connection\n");

        conn = conn_init ();
        if (conn == NULL) {
            conn_free(conn);
            continue;
        }
        conn_set_sock(conn, sock);
        conn_set_port(conn, client_addr.sin_port);
        conn_set_in_addr(conn, client_addr.sin_addr);

        struct cic_payload cic = {0};
        if (recv_payload_cic(sock, &cic) < 0) {
            conn_free(conn);
            continue;
        }
        conn_set_cic(conn, cic);

        if (dispatch_event (conn) < 0) {
            elogs("Failed to spawn thread\n");
            conn_free(conn);
            continue;
        }

        // The conn object is dispatched to a new thread and is no longer
        // our responsibility.
        conn = NULL;
    }
}

/* Set the time the server was started. This is used later for whoelsesince */
static int set_start_time(void)
{
    server.time_started = time(NULL);
    if (server.time_started == (time_t) -1)
        return -1;

    return 0;
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

    if (set_start_time () < 0) {
        elogs("Failed to set the start time\n");
        elogs("Get a better computer\n");
        free_users();
        close(server.listen_sock);
        list_free(server.connections, (void*) conn_free);
        return 1;
    }

    run_server();

    return 0;
}

