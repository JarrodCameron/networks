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
    const char *names[] = {
        [client_init_conn] = "client_init_conn",
        [server_init_conn] = "server_init_conn",
        [client_uname_auth] = "client_uname_auth",
        [server_uname_auth] = "server_uname_auth",
        [client_pword_auth] = "client_pword_auth",
        [server_pword_auth] = "server_pword_auth",
        [client_command] = "client_command",
        [server_command] = "server_command",
    };
    if (id <= 0 || id >= ARRSIZE(names))
        return "{Invalid task_id}";

    return names[id];
}

int recv_payload_sic(int sock, struct sic_payload *sic)
{
    return recv_payload(
        sock,
        server_init_conn,
        sic,
        sizeof(struct sic_payload)
    );
}

int recv_payload_cic(int sock, struct cic_payload *cic)
{
    return recv_payload(
        sock,
        client_init_conn,
        cic,
        sizeof(struct cic_payload)
    );
}

int recv_payload_cua(int sock, struct cua_payload *cua)
{
    return recv_payload(
        sock,
        client_uname_auth,
        cua,
        sizeof(struct cua_payload)
    );
}

int recv_payload_sua(int sock, struct sua_payload *sua)
{
    return recv_payload(
        sock,
        server_uname_auth,
        sua,
        sizeof(struct sua_payload)
    );
}

int recv_payload_cpa(int sock, struct cpa_payload *cpa)
{
    return recv_payload(
        sock,
        client_pword_auth,
        cpa,
        sizeof(struct cpa_payload)
    );
}

int recv_payload_spa(int sock, struct spa_payload *spa)
{
    return recv_payload(
        sock,
        server_pword_auth,
        spa,
        sizeof(struct spa_payload)
    );
}

int recv_payload_ccmd(int sock, struct ccmd_payload *ccmd)
{
    return recv_payload(
        sock,
        client_command,
        ccmd,
        sizeof(struct ccmd_payload)
    );
}

int recv_payload_scmd(int sock, struct scmd_payload *scmd)
{
    return recv_payload(
        sock,
        client_command,
        scmd,
        sizeof(struct scmd_payload)
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

int send_payload_scmd(int sock, enum status_code code)
{
    struct scmd_payload scmd = {0};
    scmd.code = code;

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

