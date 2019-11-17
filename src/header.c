/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   16/10/19 16:12               *
 *                                         *
 *******************************************/

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
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

#include "header.h"
#include "util.h"

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
    uint32_t client_id,
    void *payload
)
{
    struct header h = {
        .task_id = task_id,
        .client_id = client_id,
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
        default:                   return "{Invalid task_id}";
    }
}

#define MAKE_RECV(HEAD,TYPE)                                        \
int recv_payload_##TYPE(int sock, struct TYPE ## _payload * TYPE)   \
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

int send_payload_cbm(int sock)
{
    struct cbm_payload cbm = {0};
    cbm.dummy_ = '\0';

    return send_payload(
        sock,
        client_broad_msg,
        sizeof(cbm),
        0, // ignored
        (void **) &cbm
    );
}

int send_payload_sbm(int sock, char msg[MAX_MSG_LENGTH])
{
    struct sbm_payload sbm = {0};
    memcpy(sbm.msg, msg, MAX_MSG_LENGTH);

    return send_payload(
        sock,
        server_broad_msg,
        sizeof(sbm),
        0, // ignored
        (void **) &sbm
    );
}

int send_payload_cbon(int sock)
{
    struct cbon_payload cbon = {0};
    cbon.dummy_ = '\0';

    return send_payload(
        sock,
        client_broad_logon,
        sizeof(cbon),
        0, // ignored
        (void **) &cbon
    );
}

int send_payload_cws(int sock)
{
    struct cws_payload cws = {0};
    cws.dummy_ = '\0';

    return send_payload(
        sock,
        client_whoelse_since,
        sizeof(cws),
        0, // ignored
        (void **) &cws
    );
}

int send_payload_sbon(int sock, char name[MAX_UNAME])
{
    struct sbon_payload sbon = {0};
    memcpy(sbon.username, name, MAX_UNAME);

    return send_payload(
        sock,
        server_broad_logon,
        sizeof(sbon),
        0, // ignored
        (void **) &sbon
    );
}

int send_payload_sws(int sock, char name[MAX_UNAME])
{
    struct sws_payload sws = {0};
    memcpy(sws.username, name, MAX_UNAME);

    return send_payload(
        sock,
        server_whoelse_since,
        sizeof(sws),
        0, // ignored
        (void **) &sws
    );

}
int send_payload_ccmd(int sock, char cmd[MAX_MSG_LENGTH])
{
    struct ccmd_payload ccmd = {0};
    memcpy(ccmd.cmd, cmd, MAX_MSG_LENGTH);

    return send_payload(
        sock,
        client_command,
        sizeof(ccmd),
        0, // ignored
        (void **) &ccmd
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
        0, // ignored
        (void **) &scmd
    );
}

int send_payload_sic(int sock, enum status_code code)
{
    struct sic_payload sic = {0};
    sic.code = code;

    return send_payload(
        sock,
        server_init_conn,
        sizeof(sic),
        0, // ignored
        (void **) &sic
    );
}

int send_payload_cic(int sock, enum status_code code)
{
    struct cic_payload cic = {0};
    cic.code = code;

    return send_payload(
        sock,
        client_init_conn,
        sizeof(cic),
        0, // ignored
        (void **) &cic
    );
}

int send_payload_cua(int sock, const char uname[MAX_UNAME])
{
    struct cua_payload cua = {0};
    memcpy(cua.username, uname, MAX_UNAME);

    return send_payload(
        sock,
        client_uname_auth,
        sizeof(cua),
        0, // ignored
        (void **) &cua
    );
}

int send_payload_sua(int sock, enum status_code code)
{
    struct sua_payload sua = {0};
    sua.code = code;

    return send_payload(
        sock,
        server_uname_auth,
        sizeof(sua),
        0, // ignored
        (void **) &sua
    );
}

int send_payload_cpa(int sock, const char pword[MAX_PWORD])
{
    struct cpa_payload cpa = {0};
    memcpy(cpa.password, pword, MAX_PWORD);

    return send_payload(
        sock,
        client_pword_auth,
        sizeof(cpa),
        0, // ignored
        (void **) &cpa
    );
}

int send_payload_spa(int sock, enum status_code code)
{
    struct spa_payload spa = {0};
    spa.code = code;

    return send_payload(
        sock,
        server_pword_auth,
        sizeof(spa),
        0, // ignored
        (void **) &spa
    );
}

int send_payload_cw(int sock)
{
    struct cw_payload cw = {0};
    cw.dummy_ = '\0';

    return send_payload(
        sock,
        client_whoelse,
        sizeof(cw),
        0, // ignored
        (void **) &cw
    );
}

int send_payload_sw(int sock, char name[MAX_UNAME])
{
    struct sw_payload sw = {0};
    memcpy(sw.username, name, MAX_UNAME);

    return send_payload(
        sock,
        server_whoelse,
        sizeof(sw),
        0, // ignored
        (void **) &sw
    );
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

