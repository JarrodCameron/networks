/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   16/10/19 16:12               *
 *                                         *
 *******************************************/

#include <arpa/inet.h>
#include <assert.h>
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

int get_payload(int sock, struct header **h, void **p)
{
    struct header *head;
    void *payload;

    head = malloc(sizeof(struct header));
    if (!head)
        return -1;

    if (recv(sock, head, sizeof(struct header), 0) != sizeof(struct header)) {
        free (head);
        return -1;
    }

    if (head->data_len == 0) {
        *p = NULL;
        *h = head;
        return 0;
    }

    payload = malloc(head->data_len);

    if (payload == NULL) {
        free (head);
        return -1;
    }

    if (recv(sock, payload, head->data_len, 0) != head->data_len) {
        free (head);
        free (payload);
        return -1;
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
