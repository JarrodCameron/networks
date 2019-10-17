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

    payload = malloc(head->data_len);

    if (payload == NULL) {
        free (head);
        return -1;
    }

    if (recv(sock, payload, head->data_len, 0) != head->data_len) {
        // XXX Why the fuck is this receiving zero
        free (head);
        free (payload);
        return -1;
    }

    *p = payload;
    *h = head;

    return 0;
}

