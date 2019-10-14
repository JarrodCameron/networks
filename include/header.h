#ifndef HEADER_H
#define HEADER_H

/* This describes the header of the packets that will be sent.
 */

#include <stdint.h>

enum task_id {
    client_login_attempt = 1,   /* Client attempting to login */
    server_login_reply   = 2,   /* Server replying to client_login_attempt */
};

struct header {
    enum task_id task_id;   /* The task to be undertaken by the receiver */
    uint32_t client_id;     /* Clien't unique identifier */
    uint32_t data_len;      /* How many bytes are being transmitted */
};

#endif /* HEADER_H */
