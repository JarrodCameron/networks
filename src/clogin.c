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
#include "client.h"
#include "clogin.h"
#include "header.h"
#include "util.h"

/* Helper functions */
static enum status_code deploy_uname(int sock, char uname[MAX_UNAME]);
static enum status_code deploy_pword(int sock, char pword[MAX_PWORD]);
static void get_uname (char buff[MAX_UNAME]);
static void get_pword (char buff[MAX_PWORD]);
static enum status_code attempt_uname(int sock);
static enum status_code attempt_pword(int sock);
static void print_code_msg(enum status_code code);

/* Simple sends the username to the server and returns the code returned */
static enum status_code deploy_uname(int sock, char uname[MAX_UNAME])
{
    struct sua_payload sua = {0};

    if (send_payload_cua(sock, uname) < 0)
        return init_failed;

    if (recv_payload_sua(sock, &sua) < 0)
        return init_failed;

    return sua.code;
}

/* Simple sends the password to the server and returns the code returned */
static enum status_code deploy_pword(int sock, char pword[MAX_PWORD])
{
    struct spa_payload spa = {0};

    if (send_payload_cpa(sock, pword) < 0)
        return init_failed;

    if (recv_payload_spa(sock, &spa) < 0)
        return init_failed;

    return spa.code;
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

/* Simply handles the username check between the client and server until
 * success or a problem (blocked, bad password, etc).
 */
static enum status_code attempt_uname(int sock)
{
    enum status_code code;
    char uname[MAX_UNAME] = {0};

    get_uname(uname);
    code = deploy_uname(sock, uname);
    while (code == bad_uname) {
        printf("Invalid username!\n");
        get_uname(uname);
        code = deploy_uname(sock, uname);
    }
    client_set_uname(uname);
    return code;
}

/* Simply handles the password check between the client and server until
 * success or a problem (blocked, bad password, etc).
 */
static enum status_code attempt_pword(int sock)
{
    enum status_code code;
    char pword[MAX_PWORD] = {0};

    get_pword(pword);
    code = deploy_pword(sock, pword);
    while (code == bad_pword) {
        printf("Invalid password!\n");
        get_pword(pword);
        code = deploy_pword(sock, pword);
    }
    return code;
}

/* Print the status code in a human readable format */
static void print_code_msg(enum status_code code)
{
    switch (code) {
        case user_blocked:
            printf("You are blocked!\n");
            break;

        case already_on:
            printf("You are already logged on!\n");
            break;

        case comms_error:
        case init_failed:
            printf("Communication error between client and server\n");
            break;

        default:
            panic("Received bad code: \"%s\"(%d)\n", code_to_str(code), code);
    }
}

int attempt_login (int sock)
{
    enum status_code code;

    code = attempt_uname(sock);
    if (code != init_success) {
        printf("Failed to log in: ");
        print_code_msg(code);
        return -1;
    }

    code = attempt_pword(sock);
    if (code != init_success) {
        printf("Failed to log in: ");
        print_code_msg(code);
        return -1;
    }

    banner_logged_in();
    return 0;
}
