#ifndef UTIL_H
#define UTIL_H

/* int to string */
#define STR_(X) #X
#define STR(X) STR_(X)

/* Number of elements in array */
#define ARRSIZE(A) (sizeof(A)/sizeof(A[0]))

/* Debugging stuff */
#define check() \
    do { \
        printf("Here -> %d (%s) @ %s\n", __LINE__, __FUNCTION__, __FILE__); \
        fflush(stdout); \
    } while (0) \

#endif /* UTIL_H */
