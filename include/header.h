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
    client_login_attempt = 3,   /* Client attempting to login */
    server_login_attempt = 4,   /* Reply to client logging in */
};

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

struct sic_payload {            /* task = server_init_conn */
    enum status_code code;
};

struct cic_payload {            /* task = client_init_conn */
    char username[MAX_UNAME];   /* Clients username */
};

struct cla_payload {            /* task = client_login_attempt */
    char password[MAX_PWORD];   /* Clients password */
};

struct sla_payload {            /* task = server_login_attempt */
    enum status_code code;      /* status back to the user */
};

/* Read the payload and header from the sender, return them by reference.
 * The header and payload will be malloc'd and must be free'd by the caller.
 * Return -1 on error, zero on success */
int get_payload(int sock, struct header **head, void **payload);

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

#endif /* HEADER_H */
