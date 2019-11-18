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

/* Convert a string into a list of space seperated tokens. -1 is returned
 * if there is an internal server error (i.e. malloc error). 0 is returned
 * if there is no server error. If the users command was valid when *toks
 * is filled, otherwise *toks is equals to NULL */
struct tokens {
    char **toks;    /* Each word seperated by a space */
    int ntokens;    /* The number of tokens */
};
int tokenise(const char *line, struct tokens **toks);

/* Helper function to free the tokens struct */
void tokens_free(struct tokens *t);

/* Fill the "buffer" with null bytes for a length of "len" */
void zero_out(void *buffer, unsigned int len);

/* Create a buffer of size "len" and copy the contents of "buf" into it,
 * essentially a copy of the "buf" */
void *alloc_copy(const void *buf, unsigned int len);

/* Instead of using memcpy to copy bytes strncpy is used. This allows the full
 * buffer to be copied and zero'd out while only needing to copy the length
 * of the string */
void *safe_strndup(const void *buf, unsigned int len);

#endif /* UTIL_H */
