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
#include "user.h"
#include "util.h"

/* This is a C "hack" to pass in multiple arguments */
struct tuple {
    void *arg1;
    void *arg2;
    void *arg3;
};

/* used to probe the list of users to find the valid user.
 * u -> The data given to us in list.c
 * a -> The tuple which contains all of the arguments
 *   arg1 -> Pointer to username
 *   arg2 -> Pointer to password
 *   arg3 -> Where to return the pointer to the user struct
 */
static void user_probe (void *u, void *a)
{
    struct user *curr = u;
    struct tuple *t = a;

    struct user **ret = t->arg3;
    char *pword = t->arg2;
    char *uname = t->arg1;

    if (user_uname_cmp (curr, uname) != 0)
        return;

    if (user_pword_cmp (curr, pword) != 0)
        return;

    *ret = curr;
}

/* Returns the slr back to the user. This contains information about their
 * login attempt */
static int send_slr (int sock, struct slr_payload *slr)
{
    return send (sock, slr, sizeof(slr), 0);
}

int server_auth (int sock, struct list *users, struct cla_payload *cla)
{
    struct user *user = NULL;

    struct tuple arg = {
        .arg1 = &(cla->username),
        .arg2 = &(cla->password),
        .arg3 = &user,
    };

    printf("Recv uname: %s\n", cla->username);
    printf("Recv pword: %s\n", cla->password);
    fflush(stdout);

    list_traverse(users, user_probe, &arg);

    struct slr_payload slr = {0};

    if (user == NULL) {
        printf("Invalid login attempt\n");
        slr.resp = invalid_creds;
        slr.nth_attempt = 4;
    } else {
        printf("Valid login attempt WOO HOO!!!\n");
        slr.resp = successful;
        slr.client_id = user_getid (user);
    }

    if (send_slr (sock, &slr) < 0) {
        return -1;
    }

    return (slr.resp == successful);
}
