#ifndef UTIL_H
#define UTIL_H

#include "header.h"

#define UNUSED   __attribute__((unused))
#define NORETURN __attribute__((__noreturn__))

/* int to string */
#define STR_(X) #X
#define STR(X) STR_(X)

/* Number of elements in array */
#define ARRSIZE(A) (sizeof(A)/sizeof(A[0]))

/* Handy dandy */
#define MAX(A,B) ((A>B)?(A):B)
#define MIN(A,B) ((A<B)?(A):B)

/* Debugging stuff */
#define check()                                                             \
    do {                                                                    \
        printf("Here -> %d (%s) @ %s\n", __LINE__, __FUNCTION__, __FILE__); \
        fflush(stdout);                                                     \
    } while (0)

/* Shit has hit the fan, abort() mission */
NORETURN void panic(const char *fmt, ...);

/* Helper to free `nitems' from memory. This will allow multiple free()'s in
 * one line. This doesn't do anything important other than help my fragile
 * fingers from getting tired :( */
void bfree(const int nitems, ...);

/* Convert number of seconds to a timeval struct */
struct timeval sec_to_tv(int seconds);

/* Convert a string into a list of space seperated tokens, NULL is returned if
 * there is an error (i.e. malloc error) */
struct tokens {
    char **toks;    /* Each word seperated by a space */
    int ntokens;    /* The number of tokens */
};
struct tokens *tokenise(const char *line);

/* Helper function to free the tokens struct */
void tokens_free(struct tokens *t);

#endif /* UTIL_H */
