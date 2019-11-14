#ifndef HEADER_H
#define HEADER_H

/* This describes the header of the packets that will be sent.
 */

#include <stdint.h>

#include "config.h"
#include "status.h"

/**************************************************************************
 * The header MUST be sent on every transmissions. It communicates to the *
 * receiver what task is going to take place and how much data is being   *
 * sent. The client id uniqly idntifies the client, this prevents having  *
 * to send the username (or unique identifier) each time                  *
 **************************************************************************/

enum task_id {
    client_init_conn     = 1,   /* The client to init conn to server */
    server_init_conn     = 2,   /* Server reply when initialising connection */

    client_uname_auth    = 3,   /* Client authenticating their username */
    server_uname_auth    = 4,   /* Server replying to client_uname_auth */

    client_pword_auth    = 5,   /* Client authenticating their password */
    server_pword_auth    = 6,   /* Server replying to client_pword_auth */

    client_command       = 7,   /* Command from the client to the server */
    server_command       = 8,   /* Reply to the servers command */

    client_whoelse       = 9,   /* */
    server_whoelse       = 10,  /* Name of logged on user */

    client_whoelse_since = 11,  /* */
    server_whoelse_since = 12,  /* Logged on user since time `T' */

    client_broad_logon   = 13,  /* */
    server_broad_logon   = 14,  /* Server telling client user has logged on */
};

/* Return the task_id as a string */
const char *id_to_str(enum task_id id);

struct header {
    enum task_id task_id;   /* The task to be undertaken by the receiver */
    uint32_t client_id;     /* Clien't unique identifier */
    uint32_t data_len;      /* How many bytes are being transmitted */
};

/***************************************************************************
 * Everthing below this line is predifined payloads for when the server or *
 * client communicate to this each other. These data structures need to be *
 * shared to preferve the order of the bytes when being sent/received      *
 ***************************************************************************/

struct cic_payload {            /* task = client_init_conn */
    enum status_code code;      /* First code, should be init_success */
};

struct sic_payload {            /* task = server_init_conn */
    enum status_code code;      /* First code, should be init_success */
};

struct cua_payload {            /* task = client_uname_auth */
    char username[MAX_UNAME];   /* Clients username */
};

struct sua_payload {            /* task = server_uname_auth */
    enum status_code code;      /* Code returned back the to client */
};

struct cpa_payload {            /* task = client_pword_auth */
    char password[MAX_PWORD];   /* The client's password */
};

struct spa_payload {            /* task = server_pword_auth */
    enum status_code code;      /* The code when authenticating the user */
};

struct ccmd_payload {           /* task = client_command */
    char cmd[MAX_MSG_LENGTH];   /* The message from the client to server */
};

struct scmd_payload {           /* task = server_command */
    enum status_code code;      /* The code to send to the server */
    uint64_t extra;             /* Purpose interprited by client */
};

struct cw_payload {             /* task = client_whoelse */
    char dummy_;                /* ignored */
};

struct sw_payload {             /* task = server_whoelse */
    char username[MAX_UNAME];   /* The username of a logged in user */
};

struct cws_payload {            /* task = client_whoelse_since */
    char dummy_;                /* ignored */
};

struct sws_payload {            /* task = server_whoelse_since */
    char username[MAX_UNAME];   /* The desired username */
};

struct cbon_payload {           /* task = client_broad_logon */
    char dummy_;                /* ignored */
};

struct sbon_payload {           /* task = server_broad_logon */
    char username[MAX_UNAME];   /* User that just logged in */
};

/* Read the payload and header from the sender, return them by reference.
 * The header and payload will be malloc'd and must be free'd by the caller.
 * Return 0 on success, -errno is returned on error */
int get_payload(int sock, struct header *head, void **payload);

/* Send the payload via the socket. This simplifies the process of constructing
 * the header, sending the header, checking return type, sending payload, and
 * checking return value. Return -1 on error */
int send_payload(
    int sock,
    enum task_id task_id,
    uint32_t len,
    uint32_t client_id,
    void *payload
);

/****************************************************************************
 * Everything below this line is for sending/receiving specific payloads.   *
 * Return 0 on success, -1 on error. This makes life easier for sending and *
 * receiving specific payloads.                                             *
 ****************************************************************************/

int send_payload_cic(int sock, enum status_code code);
int recv_payload_cic(int sock, struct cic_payload *cic);

int send_payload_sic(int sock, enum status_code code);
int recv_payload_sic(int sock, struct sic_payload *sic);

int send_payload_cua(int sock, const char uname[MAX_UNAME]);
int recv_payload_cua(int sock, struct cua_payload *cua);

int send_payload_sua(int sock, enum status_code code);
int recv_payload_sua(int sock, struct sua_payload *sua);

int send_payload_cpa(int sock, const char pword[MAX_PWORD]);
int recv_payload_cpa(int sock, struct cpa_payload *cpa);

int send_payload_spa(int sock, enum status_code code);
int recv_payload_spa(int sock, struct spa_payload *spa);

int send_payload_ccmd(int sock, char cmd[MAX_MSG_LENGTH]);
int recv_payload_ccmd(int sock, struct ccmd_payload *ccmd);

int send_payload_scmd(int sock, enum status_code code, uint64_t extra);
int recv_payload_scmd(int sock, struct scmd_payload *scmd);

int send_payload_cw(int sock);
int recv_payload_cw(int sock, struct cw_payload *cw);

int send_payload_sw(int sock, char name[MAX_UNAME]);
int recv_payload_sw(int sock, struct sw_payload *sw);

int send_payload_cws(int sock);
int recv_payload_cws(int sock, struct cws_payload *cws);

int send_payload_sws(int sock, char name[MAX_UNAME]);
int recv_payload_sws(int sock, struct sws_payload *sws);

int send_payload_cbon(int sock);
int recv_payload_cbon(int sock, struct cbon_payload *cbon);

int send_payload_sbon(int sock, char username[MAX_UNAME]);
int recv_payload_sbon(int sock, struct sbon_payload *sbon);

#endif /* HEADER_H */
