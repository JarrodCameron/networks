/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   14/10/19 10:16               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "list.h"
#include "synch.h"
#include "user.h"
#include "util.h"

struct user {
    char uname[MAX_UNAME];
    char pword[MAX_PWORD];
    uint32_t id;
    bool blocked;
    bool logged_on;
    time_t log_time;    /* Epoch time since user logged on */
    struct lock *lock;
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

    ret->lock = lock_init();
    if (ret->lock == NULL) {
        free(ret);
        return NULL;
    }

    memcpy(ret->uname, uname, MAX_UNAME);
    ret->uname[MAX_UNAME-1] = '\0';

    memcpy(ret->pword, pword, MAX_PWORD);
    ret->pword[MAX_PWORD-1] = '\0';

    ret->blocked = false;
    ret->logged_on = false;

    ret->id = user_count;
    user_count += 1;

    return ret;
}

void free_user (struct user *user)
{
    // This should only be called in single threaded states.
    // If this is called with multiple threads there are
    // bigger problems to deal with.
    if (user == NULL)
        return;

    struct lock *l = user->lock;
    lock_acquire(user->lock);
    free(user);
    lock_release(user->lock);
    lock_free(l);
}

int user_uname_cmp (struct user *user, const char uname[MAX_UNAME])
{
    int ret;
    lock_acquire(user->lock);
    ret = strncmp(user->uname, uname, MAX_UNAME);
    lock_release(user->lock);
    return ret;
}

int user_pword_cmp (struct user *user, const char pword[MAX_PWORD])
{
    int ret;
    lock_acquire(user->lock);
    ret = strncmp(user->pword, pword, MAX_PWORD);
    lock_release(user->lock);
    return ret;
}

uint32_t user_getid (struct user *user)
{
    return user->id;
}

struct user *user_get_by_name (struct list *users, const char uname[MAX_UNAME])
{
    int cmp (void *item, void *arg)
    {
        struct user *user = item;
        char *name = arg;
        return strncmp(user->uname, name, MAX_UNAME);
    }
    return list_get (users, cmp, (void*) uname);
}

void user_list_dump(struct list *users)
{
    void foo(void *item, UNUSED void *arg)
    {
        struct user *user = item;
        user_dump(user);
        //printf("USER(%s, %s)\n", user->uname, user->pword);
        return;
    }
    list_traverse(users, foo, NULL);
}

void user_dump(struct user *user)
{
    printf("/----------\n");
    printf("| username: %s\n", user->uname);
    printf("| password: %s\n", user->pword);
    printf("| id:       %d\n", user->id);
    printf("| blocked:  %d\n", user->blocked);
    printf("| lock:     %p\n", user->lock);
    printf("\\----------\n");
    fflush(stdout);
}

void user_set_blocked(struct user *user)
{
    lock_acquire(user->lock);
    user->blocked = true;
    lock_release(user->lock);
}

char *user_get_uname(struct user *user)
{
    char *buff = malloc(MAX_UNAME);
    if (buff == NULL)
        return NULL;

    lock_acquire(user->lock);
    memcpy(buff, user->uname, MAX_UNAME);
    lock_release(user->lock);
    buff[MAX_UNAME-1] = '\0';
    return buff;
}

enum status_code user_log_on(struct user *user)
{
    enum status_code ret;
    assert(user);
    lock_acquire(user->lock);

    if (user->blocked == true) {
        ret = user_blocked;
    } else if (user->logged_on == true) {
        ret = already_on;
    } else {
        user->logged_on = true;
        user->log_time = time(NULL);
        ret = init_success;
    }

    lock_release(user->lock);
    return ret;
}

void user_log_off(struct user *user)
{
    lock_acquire(user->lock);
    user->logged_on = false;
    lock_release(user->lock);
}

bool user_is_logged_on(struct user *user)
{
    bool ret;

    assert(user != NULL);

    lock_acquire(user->lock);
    ret = user->logged_on;
    lock_release(user->lock);

    return ret;
}

bool user_is_blocked(struct user *user)
{
    bool ret;

    assert (user != NULL);

    lock_acquire(user->lock);
    ret = user->blocked;
    lock_release(user->lock);

    return ret;
}

struct list *user_whoelse(struct user *exception)
{
    // TODO
    (void) exception;
    return NULL;
}
