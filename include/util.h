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

/* Debugging stuff */
#define check()                                                             \
    do {                                                                    \
        printf("Here -> %d (%s) @ %s\n", __LINE__, __FUNCTION__, __FILE__); \
        fflush(stdout);                                                     \
    } while (0)

//UNUSED static const char *task_id_names[] = {
//    [client_init_conn] = "client_init_conn",
//    [client_login_attempt] = "client_login_attempt",
//};

/* Shit has hit the fan, abort() mission */
NORETURN void panic(const char *fmt, ...);

/* Helper to free `nitems' from memory. This will allow multiple free()'s in
 * one line. This doesn't do anything important other than help my fragile
 * fingers from getting tired :( */
void bfree(const int nitems, ...);

#endif /* UTIL_H */