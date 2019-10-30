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

/* Tell the user the responce depending on their username (i.e. they might be
 * blocked, have a bad username, or valid username */
static int init_conn (int sock, struct user *user)
{
    int ret;
    struct sic_payload sic = {0};

    if (user == NULL) {
        sic.code = bad_uname;
    } else if (user_is_logged_on(user) == true) {
        sic.code = already_on;
    } else if (user_is_blocked(user) == true) {
        sic.code = user_blocked;
    } else {
        sic.code = init_success;
    }

    ret = send_payload (
        sock,
        server_init_conn,
        sizeof(sic),
        0, // ignored
        (void *) &sic
    );
    return (ret < 0) ? ret : 0;
}

enum status_code login_conn
(
    int sock,
    struct list *users,
    struct cic_payload *cic,
    struct user **curr_user
)
{
    struct user *user = user_get_by_name(users, cic->username);
    int ret = init_conn(sock, user);
    if (ret < 0)
        return init_failed;

    if (user == NULL)
        return bad_uname;

    *curr_user = user;
    return init_success;
}

/* This is a helper function to reply to the client regarding the response to
 * their query */
static int send_sla(int sock, enum status_code code)
{
    int ret;
    struct sla_payload sla = {
        .code = code,
    };

    ret = send_payload(
        sock,
        server_login_attempt,
        sizeof (struct sla_payload),
        0, // ignored
        &sla
    );
    return ret;
}

enum status_code auth_user(int sock, struct user *user)
{
    assert (user != NULL);

    int nattempts;
    int ret;
    struct header *head;
    struct cla_payload *cla;
    enum status_code code;

    for (nattempts = 0; nattempts < NLOGIN_ATTEMPTS; nattempts++) {

        ret = get_payload(sock, &head, (void **) &cla);
        if (ret < 0)
            return comms_error;

        if (user_pword_cmp(user, cla->password) == 0) {
            code = user_log_on(user);
            ret = send_sla(sock, code);
            bfree(2, head, cla);
            return code;
        }
        if (nattempts < NLOGIN_ATTEMPTS - 1) {
            if (send_sla(sock, bad_pword) < 0) {
                bfree(2, head, cla);
                return comms_error;
            }
        }
        bfree(2, head, cla);
    }

    if (send_sla(sock, user_blocked) < 0)
        return comms_error;

    user_set_blocked(user);

    return user_blocked;
}
