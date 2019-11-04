/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   17/10/19 12:50               *
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
#include "list.h"
#include "logger.h"
#include "user.h"
#include "util.h"

/* Query the user for their user name. Return the "user" if the user is legit
 * otherwise return NULL. NULL is also returned in case user is blocked,
 * already logged in, etc */
static struct user *probe_user(int sock, struct list *users)
{
    struct user *user;
    struct cua_payload cua;

    user = NULL;
    while (1) {
        cua = (struct cua_payload) {0};
        if (recv_payload_cua(sock, &cua) < 0)
            return NULL;
        user = user_get_by_name(users, cua.username);

        if (user != NULL) {
            return (send_payload_sua(sock, init_success) < 0) ? NULL : user;
        } else if (send_payload_sua(sock, bad_uname) < 0) {
            return NULL;
        }
    };
}

/* Probe the user for credentials and sign in the user */
static int sign_user(int sock, struct user *user)
{
    char *name;
    enum status_code code;
    struct cpa_payload cpa;

    for (int i = 0; i < NLOGIN_ATTEMPTS; i++) {
        cpa = (struct cpa_payload) {0};

        if (recv_payload_cpa(sock, &cpa) < 0)
            return -1;

        if (user_pword_cmp(user, cpa.password) == 0) {
            code = user_log_on(user);
            if (send_payload_spa(sock, code) < 0) {
                user_log_off(user);
                return -1;
            }
            return (code == init_success) ? (0) : (-1);
        } else if (i < NLOGIN_ATTEMPTS - 1) {
            if (send_payload_spa(sock, bad_pword) < 0)
                return -1;
        }

    }

    if (user_is_logged_on(user) == true) {
        send_payload_spa(sock, already_on);
    } else {
        send_payload_spa(sock, user_blocked);
        user_set_blocked(user);

        name = user_get_uname(user);
        printf("User blocked: \"%s\"\n", name);
        free(name);
    }

    return -1;
}

struct user *auth_user(int sock, struct list *users)
{
    struct user *user;
    char *name;

    user = probe_user(sock, users);
    if (user == NULL) {
        logs("Failed to initialising connection, closing thread\n");
        return NULL;
    }

    if (sign_user(sock, user) < 0)
        return NULL;

    name = user_get_uname(user);
    logs("User logged in: \"%s\"\n", name);
    free(name);

    return user;
}
