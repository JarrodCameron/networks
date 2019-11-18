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
#include "queue.h"
#include "server.h"
#include "synch.h"
#include "user.h"
#include "util.h"

struct user {
    char uname[MAX_UNAME];
    char pword[MAX_PWORD];
    uint32_t id;
    bool blocked;
    bool logged_on;
    time_t log_time;            /* Epoch time since user logged on */
    struct lock *lock;          /* Prevent race conditions */
    struct list *block_list;    /* The list of blocked users, all char*'s */
    struct queue *backlog;      /* Backlog of messages to send the client when
                                 * they log in */
};

/* Unique counter for all of the users, so that they don't need to be
 * identified by username each time */
static uint32_t user_count = 1;

/* Helper functions */
static int name_cmp(void *n1, void *n2);
static void rm_name_from_list(struct list *list, const char *name);
static bool already_blocked(struct list *block_list, const char *victim);
static struct sdmm_payload *generate_sdmm(const char *name, const char *msg);
static int add_to_block_list(struct list *block_list, const char *name);
static bool valid_whoelse(struct user *user, struct user *execption);
static int add_username_to_list(struct list *name_list, struct user *curr_user);
static bool valid_whoelsesince (struct user *curr, struct user *exce, time_t offt);
static bool has_logged_on_recently(struct user *user, time_t off_time);
static int uname_cmp_wrapper(void *n1, void *n2);
static bool has_logged_on(struct user *user);

struct user *user_init (const char uname[MAX_UNAME], const char pword[MAX_PWORD])
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

    ret->block_list = list_init();
    if (ret->block_list == NULL) {
        lock_free(ret->lock);
        free(ret);
        return NULL;
    }

    ret->backlog = queue_init();
    if (ret->backlog == NULL) {
        lock_free(ret->lock);
        list_free(ret->block_list, NULL);
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

void user_free (struct user *user)
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

bool user_equal(struct user *user1, struct user *user2)
{
    if (user_uname_cmp(user1, user2->uname) != 0)
        return false;

    if (user_pword_cmp(user1, user2->pword) != 0)
        return false;

    return true;
}

struct list *user_whoelse(struct list *users, struct user *exception)
{
    struct iter *iter = NULL;
    struct list *name_list = NULL;

    iter = list_iter_init(users);
    if (iter == NULL)
        goto user_whoelse_error;

    name_list = list_init();
    if (name_list == NULL)
        goto user_whoelse_error;

    while(iter_has_next(iter) == true) {
        struct user *curr_user = iter_get(iter);

        if (valid_whoelse(curr_user, exception) == false) {
            iter_next(iter);
            continue;
        }

        if (add_username_to_list(name_list, curr_user) < 0)
            goto user_whoelse_error;

        // Yeahhhh boiiii, This is C++ in the wild
        iter_next(iter);
    }

    iter_free(iter);
    return name_list;

user_whoelse_error:
    iter_free(iter);
    list_free(name_list, free);
    return NULL;
}

struct list *user_whoelsesince
(
    struct list *users,
    struct user *exception,
    time_t off_time
)
{
    (void) off_time;
    struct iter *iter = NULL;
    struct list *name_list = NULL;

    iter = list_iter_init(users);
    if (iter == NULL)
        goto user_whoelsesince_error;

    name_list = list_init();
    if (name_list == NULL)
        goto user_whoelsesince_error;

    while(iter_has_next(iter) == true) {
        struct user *curr_user = iter_get(iter);

        if (valid_whoelsesince(curr_user, exception, off_time) == false) {
            iter_next(iter);
            continue;
        }

        if (add_username_to_list(name_list, curr_user) < 0)
            goto user_whoelsesince_error;

        iter_next(iter);
    }

    iter_free(iter);
    return name_list;

user_whoelsesince_error:
    iter_free(iter);
    list_free(name_list, free);
    return NULL;
}

enum status_code user_block
(
    struct list *users,
    struct user *blocker,
    const char *victim_name
)
{
    struct user *victim = user_get_by_name(users, victim_name);
    if (victim == NULL)
        return bad_uname;

    if (user_uname_cmp(blocker, victim_name) == 0)
        return dup_error;

    if (already_blocked(blocker->block_list, victim_name) == true)
        return user_blocked;

    if (add_to_block_list(blocker->block_list, victim_name) < 0)
        return server_error;

    return task_success;
}

enum status_code user_unblock
(
    struct list *users,
    struct user *unblocker,
    const char *victim_name
)
{
    struct user *victim = user_get_by_name(users, victim_name);
    if (victim == NULL)
        return bad_uname;

    if (user_uname_cmp(unblocker, victim_name) == 0)
        return dup_error;

    if (already_blocked(unblocker->block_list, victim_name) == false)
        return user_unblocked;

    rm_name_from_list(unblocker->block_list, victim_name);
    return task_success;
}

bool user_on_blocklist (struct user *reciver, struct user *sender)
{
    struct user *user = list_get(reciver->block_list, name_cmp, sender->uname);
    return (user != NULL);
}

int user_add_to_backlog(struct user *user, const char *name, const char *msg)
{
    struct sdmm_payload *sdmm = generate_sdmm(name, msg);
    if (sdmm == NULL)
        return -1;

    if (queue_push(user->backlog, sdmm) < 0)
        return -1;

    return 0;
}

int user_get_backlog_len(struct user *user)
{
    assert(user != NULL);
    return queue_len(user->backlog);
}

struct sdmm_payload *user_pop_backlog(struct user *user)
{
    assert(user != NULL);
    return queue_pop(user->backlog);
}

/* Given the name (of the sender) and the message generate a sdmm_payload */
static struct sdmm_payload *generate_sdmm(const char *name, const char *msg)
{
    struct sdmm_payload *sdmm = malloc(sizeof(struct sdmm_payload));
    if (sdmm == NULL)
        return NULL;

    *sdmm = (struct sdmm_payload) {0};

    strncpy(sdmm->sender, name, MAX_UNAME);
    strncpy(sdmm->msg, msg, MAX_MSG_LENGTH);
    return sdmm;
}

/* Add the victim_name to the block list, return -1 on error, otherwise
 * return 0 */
static int add_to_block_list(struct list *block_list, const char *name)
{
    char *name_dup = alloc_copy(name, MAX_UNAME);
    if (name_dup == NULL)
        return -1;

    if (list_add(block_list, name_dup) < 0) {
        free(name_dup);
        return -1;
    }

    return 0;
}

/* Return true of the "victim" is in the "blocked_list" */
static bool already_blocked(struct list *block_list, const char *victim)
{
    return !!list_get(block_list, uname_cmp_wrapper, (void *) victim);
}

/* Wrapper for some yummy void pointers to compare two usernames */
static int uname_cmp_wrapper(void *n1, void *n2)
{
    return strncmp(n1, n2, MAX_UNAME);
}

/* Return true if the "curr_user" is valid for the whoelsesince command */
static bool valid_whoelsesince
(
    struct user *curr_user,
    struct user *exception,
    time_t off_time
)
{
    if (user_equal(curr_user, exception) == true)
        return false;

    if (server_uptime() < off_time && has_logged_on(curr_user) == true) {
        return true;
    }

    if (has_logged_on_recently(curr_user, off_time) == true) {
        return true;
    }

    return false;
}

/* Return true if the user has logged on, return false if the user has
 * never logged on */
static bool has_logged_on(struct user *user)
{
    assert(user != NULL);
    return (user->log_time != 0);
}

/* Return true of the user has logged on in the last "off_time" seconds,
 * otherwise return false */
static bool has_logged_on_recently(struct user *user, time_t off_time)
{
    assert(user != NULL);
    time_t time_since_log_on = time(NULL) - user->log_time;
    return (time_since_log_on <= off_time);
}

/* Add the username of curr_user to the name list. Return -1 on error,
 * zero is returned on success */
static int add_username_to_list(struct list *name_list, struct user *curr_user)
{
    char *name = malloc(MAX_UNAME);
    if (name == NULL)
        return -1;

    memcpy(name, curr_user->uname, MAX_UNAME);

    if (list_add(name_list, name) < 0) {
        free(name);
        return -1;
    }

    return 0;
}

/* Remove name from list of names. The list is a list of char*'s, NOT users */
static void rm_name_from_list(struct list *list, const char *name)
{
    char *old_name = list_rm(list, (void *) name, name_cmp);
    assert(old_name != NULL);
    free(old_name);
}

/* Return true if the user is valid for the whoelse command, otherwise
 * return false */
static bool valid_whoelse(struct user *curr_user, struct user *exception)
{

    if (user_is_logged_on(curr_user) == false)
        return false;

    if (user_equal(curr_user, exception) == true)
        return false;

    return true;
}

/* Taking two users as void pointers, return 0 if they are equals, otherwise
 * 1 is return (to inidicate they are different) */
static int name_cmp(void *u1, void *u2)
{
    return strncmp(u1, u2, MAX_UNAME);
}
