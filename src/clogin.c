/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   17/10/19 14:21               *
 *                                         *
 *******************************************/

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "header.h"

/* Helper functions */
static void get_cla_payload(struct cla_payload *cla);

int attempt_login (int sock)
{
    struct header h = {
        .task_id   = client_login_attempt,
        .client_id = 0, /* ignored */
        .data_len  = sizeof(struct cla_payload),
    };

    struct cla_payload cla = {0};
    get_cla_payload(&cla);

    if (send (sock, &h, sizeof(h), 0) < 0)
        return -1;

    if (send (sock, &cla, sizeof(cla), 0) < 0)
        return -1;

    // TODO Check the returns and stuff
    return -1;
}

/* Simply prompts the user for a username and password */
static void get_cla_payload(struct cla_payload *cla)
{
    int ret;

    memset(cla->username, 0, MAX_UNAME);
    memset(cla->password, 0, MAX_PWORD);

    printf("Enter your username !!!\n");

    // Need to flush because glibc will buffer writes without '\n'

    printf("Username: ");
    fflush(stdout);
    ret = read(0, cla->username, MAX_UNAME-1);
    while (ret <= 1) {
        printf("Invalid username!\n");
        printf("Username: ");
        fflush(stdout);
        ret = read(0, cla->username, MAX_UNAME-1);
    }

    printf("Password: ");
    fflush(stdout);
    ret = read(0, cla->password, MAX_PWORD-1);
    while (ret <= 1) {
        printf("Invalid password!\n");
        printf("Password: ");
        fflush(stdout);
        ret = read(0, cla->password, MAX_PWORD-1);
    }

    for (int i = 0; i < MAX_UNAME; i++) {
        if (cla->username[i] == '\n' || cla->username[i] == '\r') {
            cla->username[i] = '\0';
            break;
        }
    }

    for (int i = 0; i < MAX_PWORD; i++) {
        if (cla->password[i] == '\n' || cla->password[i] == '\r') {
            cla->password[i] = '\0';
            break;
        }
    }
}

