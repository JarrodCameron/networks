/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   16/10/19 16:12               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"

/* Helper functions */
static int recv_payload(int, enum task_id, void *, uint32_t);

int get_payload(int sock, struct header *h, void **p)
{
    struct header head = {0};
    void *payload;

    if (recv(sock, &head, sizeof(struct header), 0) != sizeof(struct header)) {
        return -errno;
    }

    if (head.data_len == 0) {
        *p = NULL;
        *h = head;
        return 0;
    }

    payload = malloc(head.data_len);
    if (payload == NULL) {
        return -1;
    }

    if (recv(sock, payload, head.data_len, 0) != head.data_len) {
        free (payload);
        return -errno;
    }

    *p = payload;
    *h = head;
    return 0;
}

int send_payload(
    int sock,
    enum task_id task_id,
    uint32_t len,
    void *payload
)
{
    struct header h = {
        .task_id = task_id,
        .data_len = len,
    };

    uint32_t total_len = sizeof(struct header) + len;

    char *total_payload = malloc(total_len);
    if (!total_payload)
        return -1;

    // It's important to send the header and payload at the same time just in
    // case two or more payloads are sent at the same time.
    memcpy(total_payload, &h, sizeof(h));
    memcpy(&total_payload[sizeof(h)], payload, len);

    if (send(sock, total_payload, total_len, 0) != total_len) {
        free (total_payload);
        return -1;
    }

    free (total_payload);
    return 0;

}

const char *id_to_str(enum task_id id)
{
    switch(id) {
        case client_init_conn:     return "client_init_conn";
        case server_init_conn:     return "server_init_conn";
        case client_uname_auth:    return "client_uname_auth";
        case server_uname_auth:    return "server_uname_auth";
        case client_pword_auth:    return "client_pword_auth";
        case server_pword_auth:    return "server_pword_auth";
        case client_command:       return "client_command";
        case server_command:       return "server_command";
        case client_whoelse:       return "client_whoelse";
        case server_whoelse:       return "server_whoelse";
        case client_whoelse_since: return "client_whoelse_since";
        case server_whoelse_since: return "server_whoelse_since";
        case client_broad_logon:   return "client_broad_logon";
        case server_broad_logon:   return "server_broad_logon";
        case client_broad_msg:     return "client_broad_msg";
        case server_broad_msg:     return "server_broad_msg";
        case client_block_user:    return "client_block_user";
        case server_block_user:    return "server_block_user";
        case client_dm_response:   return "client_dm_response";
        case server_dm_response:   return "server_dm_response";
        case client_dm_msg:        return "client_dm_msg";
        case server_dm_msg:        return "server_dm_msg";
        case client_unblock_user:  return "client_unblock_user";
        case server_unblock_user:  return "server_unblock_user";
        case client_broad_logoff:  return "client_broad_logoff";
        case server_broad_logoff:  return "server_broad_logoff";
        case client_start_private: return "client_start_private";
        case server_start_private: return "server_start_private";
        case ptop_command:         return "ptop_command";
        case ptop_init_conn:       return "ptop_init_conn";
        case ptop_handshake:       return "ptop_handshake";
        default:                   return "{Invalid task_id}";
    }
}

/* Helper function for all of the recv_* functions */
static int recv_payload
(
    int sock,
    enum task_id task_id,
    void *payload,
    uint32_t size
)
{
    struct header head = {0};
    if (recv(sock, &head, sizeof(head), 0) != sizeof(head))
        return -1;

    assert(head.task_id == task_id);
    assert(head.data_len == size);

    if (recv(sock, payload, head.data_len, 0) != head.data_len)
        return -1;

    return 0;
}


/* Simplify the receive process since they are all the same */
#define MAKE_RECV(HEAD,TYPE)                                        \
int recv_payload_ ## TYPE(int sock, struct TYPE ## _payload * TYPE) \
{                                                                   \
    return recv_payload(                                            \
        sock,                                                       \
        HEAD,                                                       \
        TYPE,                                                       \
        sizeof(struct TYPE ## _payload)                             \
    );                                                              \
}
MAKE_RECV(server_init_conn, sic)
MAKE_RECV(client_init_conn, cic)
MAKE_RECV(client_uname_auth, cua)
MAKE_RECV(server_uname_auth, sua)
MAKE_RECV(client_pword_auth, cpa)
MAKE_RECV(server_pword_auth, spa)
MAKE_RECV(client_command, ccmd)
MAKE_RECV(server_command, scmd)
MAKE_RECV(client_whoelse, cw)
MAKE_RECV(server_whoelse, sw)
MAKE_RECV(client_whoelse_since, cws)
MAKE_RECV(server_whoelse_since, sws)
MAKE_RECV(client_broad_logon, cbon)
MAKE_RECV(server_broad_logon, sbon)
MAKE_RECV(client_broad_msg, cbm)
MAKE_RECV(server_broad_msg, sbm)
MAKE_RECV(client_block_user, cbu)
MAKE_RECV(server_block_user, sbu)
MAKE_RECV(client_dm_response, cdmr)
MAKE_RECV(server_dm_response, sdmr)
MAKE_RECV(client_dm_msg, cdmm)
MAKE_RECV(server_dm_msg, sdmm)
MAKE_RECV(client_unblock_user, cuu)
MAKE_RECV(server_unblock_user, suu)
MAKE_RECV(client_broad_logoff, cbof)
MAKE_RECV(server_broad_logoff, sbof)
MAKE_RECV(client_start_private, csp)
MAKE_RECV(server_start_private, ssp)
MAKE_RECV(ptop_command, pcmd)
MAKE_RECV(ptop_init_conn, pic)
MAKE_RECV(ptop_handshake, phs)

/* Simplify the send process for dummy structs */
#define MAKE_SEND_DUMMY(HEAD,TYPE)          \
int send_payload_ ## TYPE (int sock)        \
{                                           \
    struct TYPE ## _payload TYPE = {0};     \
    TYPE.dummy_ = '\0';                     \
                                            \
    return send_payload(                    \
        sock,                               \
        HEAD,                               \
        sizeof(struct TYPE ## _payload),    \
        (void **) & TYPE                    \
    );                                      \
}
MAKE_SEND_DUMMY(client_broad_msg, cbm)
MAKE_SEND_DUMMY(client_block_user, cbu)
MAKE_SEND_DUMMY(client_broad_logon, cbon)
MAKE_SEND_DUMMY(client_whoelse_since, cws)
MAKE_SEND_DUMMY(client_whoelse, cw)
MAKE_SEND_DUMMY(client_dm_response, cdmr)
MAKE_SEND_DUMMY(client_dm_msg, cdmm)
MAKE_SEND_DUMMY(client_unblock_user, cuu)
MAKE_SEND_DUMMY(client_broad_logoff, cbof)
MAKE_SEND_DUMMY(client_start_private, csp)

/* Simplify the send process for structs with code_status's */
#define MAKE_SEND_CODE(HEAD,TYPE)                           \
int send_payload_ ## TYPE (int sock, enum status_code code) \
{                                                           \
    struct TYPE ## _payload TYPE = {0};                     \
    TYPE.code = code;                                       \
                                                            \
    return send_payload(                                    \
        sock,                                               \
        HEAD,                                               \
        sizeof(struct TYPE ## _payload),                    \
        (void **) & TYPE                                    \
    );                                                      \
}
MAKE_SEND_CODE(server_block_user, sbu)
MAKE_SEND_CODE(server_init_conn, sic)
MAKE_SEND_CODE(client_init_conn, cic)
MAKE_SEND_CODE(server_uname_auth, sua)
MAKE_SEND_CODE(server_pword_auth, spa)
MAKE_SEND_CODE(server_dm_response, sdmr)
MAKE_SEND_CODE(server_unblock_user, suu)

/* Simplify the send process for structs with buffers */
#define MAKE_SEND_BUFF(HEAD,TYPE,BUFF_NAME,BUFF_SIZE)           \
int send_payload_ ## TYPE (int sock, const char BUFF_NAME[BUFF_SIZE]) \
{                                                               \
    struct TYPE ## _payload TYPE = {0};                         \
    memcpy(TYPE . BUFF_NAME, BUFF_NAME, BUFF_SIZE);             \
                                                                \
    return send_payload(                                        \
        sock,                                                   \
        HEAD,                                                   \
        sizeof(struct TYPE ## _payload),                        \
        (void **) & TYPE                                        \
    );                                                          \
}
MAKE_SEND_BUFF(server_broad_msg, sbm, msg, MAX_MSG_LENGTH)
MAKE_SEND_BUFF(server_broad_logon, sbon, username, MAX_UNAME)
MAKE_SEND_BUFF(server_whoelse_since, sws, username, MAX_UNAME)
MAKE_SEND_BUFF(client_command, ccmd, cmd, MAX_MSG_LENGTH)
MAKE_SEND_BUFF(client_uname_auth, cua, username, MAX_UNAME)
MAKE_SEND_BUFF(client_pword_auth, cpa, password, MAX_PWORD)
MAKE_SEND_BUFF(server_whoelse, sw, username, MAX_UNAME)
MAKE_SEND_BUFF(server_broad_logoff, sbof, name, MAX_UNAME)
MAKE_SEND_BUFF(ptop_command, pcmd, cmd, MAX_MSG_LENGTH)
MAKE_SEND_BUFF(ptop_handshake, phs, name, MAX_UNAME)

int send_payload_sdmm
(
    int sock,
    const char sender[MAX_UNAME],
    const char msg[MAX_MSG_LENGTH]
)
{
    struct sdmm_payload sdmm = {0};
    memcpy(sdmm.sender, sender, MAX_UNAME);
    memcpy(sdmm.msg, msg, MAX_MSG_LENGTH);

    return send_payload(
        sock,
        server_dm_msg,
        sizeof(struct sdmm_payload),
        (void **) &sdmm
    );
}

int send_payload_scmd(int sock, enum status_code code, uint64_t extra)
{
    struct scmd_payload scmd = {0};
    scmd.code = code;
    scmd.extra = extra;

    return send_payload(
        sock,
        server_command,
        sizeof(scmd),
        (void **) &scmd
    );
}

int send_payload_ssp
(
    int sock,
    enum status_code code,
    unsigned short port,
    struct in_addr addr
)
{
    struct ssp_payload ssp = {0};
    ssp.port = port;
    ssp.addr = addr;
    ssp.code = code;

    return send_payload(
        sock,
        server_start_private,
        sizeof(ssp),
        (void **) &ssp
    );
}

int send_pic_payload(int sock, unsigned short port, struct in_addr addr)
{
    struct pic_payload pic = {0};
    pic.port = port;
    pic.addr = addr;

    return send_payload(
        sock,
        ptop_init_conn,
        sizeof(pic),
        (void **) &pic
    );

    return 0;
}
