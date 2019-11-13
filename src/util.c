/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   21/10/19 13:47               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"

/* The name of each valid command */
static const char *cmd_names[] = {
    "message", "broadcast", "whoelsesince", "whoelse", "block", "unblock",
    "logout", "startprivate", "private", "stopprivate",
};

/* The max number of seperators for cmd_names[i] */
static int cmd_max_seps[] = {
    3, 2, 2, 1, 2, 2, 1, 2, 3, 2,
};

_Static_assert(
    ARRSIZE(cmd_names) == ARRSIZE(cmd_max_seps),
    "Seperators and names don't match!"
);

/* Helper functions */
static const char *get_first_non_space(const char *line);
static int get_max_seps(const char *line);

NORETURN void panic(const char *fmt, ...)
{
    va_list ap;

#ifdef I_AM_SERVER
    fprintf(stderr, ">>> SERVER PANIC <<<\n");
#endif /* I_AM_SERVER */

#ifdef I_AM_CLIENT
    fprintf(stderr, ">>> CLIENT PANIC <<<\n");
#endif /* I_AM_CLIENT */

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "abort()\n");
    abort();
}

void bfree(const int nitems, ...)
{
    va_list ap;
    void *item;
    va_start(ap, nitems);
    for (int i = 0; i < nitems; i++) {
        item = va_arg(ap, void *);
        free (item);
    }
    va_end(ap);
}

struct timeval sec_to_tv(int seconds)
{
    struct timeval ret = {0};
    ret.tv_sec = seconds;
    return ret;
}

/* This function is a modified version of Jas's tokenise from the mysh
 * assignment (Ass 2) from COMP1521, 18s2. If this function has bugs I
 * am re-doing my assignment in python on the last day. */
int tokenise(const char *line, struct tokens **toks)
{
    if (line == NULL || line[0] == '\0')
        return 0;

    *toks = NULL;

    struct tokens *tokens = malloc(sizeof(struct tokens));
    if (tokens == NULL)
        return -1;

    *tokens = (struct tokens) {0};

    char *temp = strdup(line);
    if (temp == NULL) {
        tokens_free(tokens);
        return -1;
    }

    int max_seps = get_max_seps(temp);
    if (max_seps < 0) {
        // Command doesnt exist
        tokens_free(tokens);
        free(temp);
        return 0;
    }

    char *saveptr = NULL;
    strtok_r(temp, " ", &saveptr);
    int n = 1;
    while (n < max_seps && strtok_r(NULL, " ", &saveptr) != NULL)
        n += 1;

    free(temp);

    if (n != max_seps) {
        // Only some of the args were given
        tokens_free(tokens);
        return 0;
    }

    char **strings = calloc(n, sizeof(char *));
    if (strings == NULL) {
        tokens_free(tokens);
        return -1;
    }

    temp = strdup(line);

    int i = 0;
    char *next = strtok_r(temp, " ", &saveptr);
    strings[i++] = strdup(next);
    while (i < n-1 && (next = strtok_r(NULL, " ", &saveptr)) != NULL)
        strings[i++] = strdup(next);

    if (i != n) {
        next += strlen(next) + 1;
        strings[i] = strdup(next);
    }
    free(temp);

    tokens->toks = strings;
    tokens->ntokens = n;

    *toks = tokens;

    return 0;
}

/* Return the first legit character of a line. Non-legit characters are spaces
 * and null bytes */
static const char *get_first_non_space(const char *line)
{
    const char *ret = line;
    while (*ret == ' ' || *ret == '\0')
        ret++;

    if (*ret == '\0')
        return NULL;

    return ret;
}

/* Return the max number of seperators for a line. If the line is not a valid
 * command the NULL is returned */
static int get_max_seps(const char *line)
{
    const char *start = get_first_non_space(line);
    if (start == NULL)
        return -1;

    assert(ARRSIZE(cmd_names) == ARRSIZE(cmd_max_seps));

    unsigned int i;
    for (i = 0; i < ARRSIZE(cmd_names); i++) {
        const char *name = cmd_names[i];
        if (strncmp(start, name, strlen(name)) == 0) {
            return cmd_max_seps[i];
        }
    }
    return -1;
}

void tokens_free(struct tokens *t) {
    if (t == NULL)
        return;

    for (int i = 0; i < t->ntokens; i++) {
        free(t->toks[i]);
    }

    free(t->toks);
    free(t);
}

void zero_out(void *buffer, unsigned int len)
{
    assert(buffer != NULL);
    char *array = buffer;
    unsigned int i;
    for (i = 0; i < len; i++) {
        array[i] = '\0';
    }
}
