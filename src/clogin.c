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

#include "banner.h"
#include "clogin.h"
#include "header.h"
#include "util.h"

/* Helper functions */
static enum status_code conn_to_server (int sock, char uname[MAX_UNAME]);
static int handle_code (enum status_code code);
static void get_uname (char buff[MAX_UNAME]);
static void get_pword (char buff[MAX_PWORD]);

/* Just initialise the connection to the server. This doesn't involve TCP but
 * is an application layer protocol. This will send the header with
 * task_id = client_init_conn. */
static enum status_code conn_to_server (int sock, char uname[MAX_UNAME])
{
    int ret;
    struct cic_payload cic = {0};
    struct sic_payload *sic = NULL;
    struct header *header;

    memcpy(cic.username, uname, MAX_UNAME);

    ret = send_payload(
        sock,
        client_init_conn,
        sizeof(cic),
        0, // ignored
        &cic
    );
    if (ret < 0)
        return init_failed;

    ret = get_payload(
        sock,
        &header,
        (void*) &sic
    );
    if (ret < 0)
        return init_failed;

    enum status_code code = sic->code;

    free(header);
    free(sic);

    return code;
}

/* Prompt the user for their username */
static void get_uname (char buff[MAX_UNAME])
{
    int ret = 0;
    printf("Username: ");
    fflush(stdout);
    ret = read(0, buff, MAX_UNAME-1);

    while (ret <= 1) {
        printf("Invalid username!\n");
        printf("Username: ");
        fflush(stdout);
        ret = read(0, buff, MAX_UNAME-1);
    }

    if (buff[ret-1] == '\n' || buff[ret-1] == '\r')
        buff[ret-1] = '\0';

}

/* Prompt the user for their password */
static void get_pword (char buff[MAX_PWORD])
{
    int ret = 0;
    printf("Password: ");
    fflush(stdout);
    ret = read(0, buff, MAX_PWORD-1);

    while (ret <= 0) {
        printf("Invalid password!\n");
        printf("Password: ");
        fflush(stdout);
        ret = read(0, buff, MAX_PWORD-1);
    }

    if (buff[ret-1] == '\n' || buff[ret-1] == '\r')
        buff[ret-1] = '\0';
}

/* Authenticate the user with their password */
static enum status_code pword_auth(int sock)
{
    struct cla_payload cla = {0};
    struct sla_payload *sla = NULL;
    struct header *head = NULL;
    enum status_code code;
    int ret;

    get_pword(cla.password);

    ret = send_payload(
        sock,
        client_login_attempt,
        sizeof(struct cla_payload),
        0, // ignored
        &cla
    );
    if (ret < 0)
        return init_failed;

    ret = get_payload (
        sock,
        &head,
        (void **) &sla
    );
    if (ret < 0)
        return init_failed;

    code = sla->code;

    free (head);
    free (sla);

    return code;
}

int attempt_login (int sock)
{
    enum status_code code;
    char uname[MAX_UNAME] = {0};

    get_uname(uname);
    code = conn_to_server (sock, uname);
    if (code == init_failed) {
        return handle_code (code);
    }

    while (code == bad_uname) {
        printf("Invalid username!\n");
        get_uname(uname);
        code = conn_to_server (sock, uname);
    }

    if (code != init_success) {
        return handle_code (code);
    }

    code = pword_auth(sock);
    while (code == bad_pword) {
        printf("Invalid password!\n");
        code = pword_auth(sock);
    }

    return handle_code (code);
}

/* This will handle what do do with the code after the logging is is done,
 * mostly used to display error messages and welcome messages
 *   0 -> Sucess
 *  -1 -> The client sould kill itself
 * */
static int handle_code (enum status_code code)
{
    switch (code) {
        case init_success:
            banner_logged_in();
            return 0;

        case comms_error:
        case init_failed:
            printf("Communication error between client and server\n");
            return -1;

        case user_blocked:
            printf("Sorry mate, your'e blocked!\n");
            return -1;

        case already_on:
            printf("It appears you are already logged on\n");
            printf("You can only be logged on once!\n");
            return -1;

        default:
            panic("Unkown code: \"%s\"(%d)\n", code_to_str(code), code);
    }
}


