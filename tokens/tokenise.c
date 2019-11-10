/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   07/11/19 12:49               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define check() printf("Here -> %d (%s)\n", __LINE__, __FUNCTION__)
#define ARRSIZE(A) (sizeof(A)/sizeof(A[0]))

/* Convert a string into a list of space seperated tokens, NULL is returned if
 * there is an error (i.e. malloc error) */
struct tokens {
    char **toks;    /* Each word seperated by a space */
    int ntokens;    /* The number of tokens */
};
struct tokens *tokenise(const char *line);
static const char *get_first_non_space(const char *line);
static int get_max_seps(const char *line);
void tokens_free(struct tokens *t);

struct tokens *tokenise(const char *line)
{
    if (line == NULL)
        return NULL;

    struct tokens *tokens = malloc(sizeof(struct tokens));
    if (tokens == NULL)
        return NULL;

    *tokens = (struct tokens) {0};

    char *temp = strdup(line);
    if (temp == NULL) {
        tokens_free(tokens);
        return NULL;
    }

    int max_seps = get_max_seps(temp);
    if (max_seps < 0) {
        // Command doesnt exist
        tokens_free(tokens);
        return NULL;
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
        return NULL;
    }

    char **strings = calloc(n, sizeof(char *));
    assert(strings != NULL);

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

    return tokens;
}

static const char *get_first_non_space(const char *line)
{
    const char *ret = line;
    while (*ret == ' ' || *ret == '\0')
        ret++;

    if (*ret == '\0')
        return NULL;

    return ret;
}

// The name of each valid command
static const char *cmd_names[] = {
    "message", "broadcast", "whoelsesince", "whoelse", "block", "unblock",
    "logout", "startprivate", "private", "stopprivate",
};

// The max number of seperators for cmd_names[i]
static int cmd_max_seps[] = {
    3, 2, 2, 1, 2, 2, 1, 2, 3, 2,
};

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

// Modified Jas's code from 18s2 Assignment 2
int main (int argc, char **argv) {
    assert(argc == 2);

    struct tokens *tokens = tokenise(argv[1]);

    if (tokens == NULL) {
        printf("Received null\n");
        return 0;
    }

    for (int i = 0; i < tokens->ntokens; i++) {
        printf("strings[%d] -> \"%s\"\n", i, tokens->toks[i]);
    }

    tokens_free(tokens);

    return 0;
}
