#ifndef HEADER_H
#define HEADER_H

/* This describes the header of the packets that will be sent.
 */

#include <stdint.h>

#include "config.h"

/**************************************************************************
 * The header MUST be sent on every transmissions. It communicates to the *
 * receiver what task is going to take place and how much data is being   *
 * sent. The client id uniqly idntifies the client, this prevents having  *
 * to send the username (or unique identifier) each time                  *
 **************************************************************************/

enum task_id {
    client_login_attempt = 1,   /* Client attempting to login */
    server_login_reply   = 2,   /* Server replying to client_login_attempt */
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

struct cla_payload {            /* task = client_login_attempt */
    char username[MAX_UNAME];   /* Field to send the username */
    char password[MAX_PWORD];   /* Field to send the password */
};

struct slr_payload {
    enum {
        successful,             /* Auth was valid, client logged in */
        invalid_creds,          /* Auth failed due to bad username/password */
    } resp;                     /* ID of a response given by the server */
    union {
        uint32_t client_id;     /* Unique id for user (resp==successful) */
        int nth_attempt;        /* n attempts so far (resp=invalid_creds) */
    };
};

/* Read the payload and header from the sender, return them by reference.
 * The header and payload will be malloc'd and must be free'd by the caller.
 * Return -1 on error, zero on success */
int get_payload(int sock, struct header **head, void **payload);

#endif /* HEADER_H */
