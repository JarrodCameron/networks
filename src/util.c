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

#include "util.h"

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
