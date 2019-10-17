/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   14/10/19 10:16               *
 *                                         *
 *******************************************/

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "user.h"
#include "util.h"

struct user {
    char uname[MAX_UNAME];
    char pword[MAX_PWORD];
    uint32_t id;
};

/* Unique counter for all of the users, so that they don't need to be
 * identified by username each time */
static uint32_t user_count = 1;

struct user *init_user (const char uname[MAX_UNAME], const char pword[MAX_PWORD])
{
    struct user *ret = malloc(sizeof(struct user));
    if (!ret)
        return NULL;

    *ret = (struct user) {0};

    memcpy(ret->uname, uname, MAX_UNAME);
    ret->uname[MAX_UNAME-1] = '\0';

    memcpy(ret->pword, pword, MAX_PWORD);
    ret->pword[MAX_PWORD-1] = '\0';

    ret->id = user_count;
    user_count += 1;

    return ret;
}

void free_user (struct user *user)
{
    if (user == NULL)
        return;

    free(user);
}

int user_uname_cmp (struct user *user, const char uname[MAX_UNAME])
{
    return strncmp(user->uname, uname, MAX_UNAME);
}

int user_pword_cmp (struct user *user, const char pword[MAX_PWORD])
{
    return strncmp(user->pword, pword, MAX_PWORD);
}

uint32_t user_getid (struct user *user)
{
    return user->id;
}
