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

struct timeval sec_to_tv(int seconds)
{
    struct timeval ret = {0};
    ret.tv_sec = seconds;
    return ret;
}

struct tokens *tokenise(const char *line)
{
    if (line == NULL)
        return NULL;
    struct tokens *ret = malloc(sizeof(struct tokens));
    *ret = (struct tokens) {0};
    // TODO Implement me please
    return NULL;
}

// From John Sheaphard ////////////////////////////////////////////////////////
//           // tokenise: split a string around a set of separators
//           // create an array of separate strings
//           // final array element contains NULL
//           char **tokenise(char *str, char *sep) {
//                   // temp copy of string, because strtok() mangles it
//                   char *tmp;
//                   // count tokens
//                   tmp = strdup(str);
//                   int n = 0;
//                   strtok(tmp, sep); n++;
//                   while (strtok(NULL, sep) != NULL) n++;
//                   free(tmp);
//                   // allocate array for argv strings
//                   char **strings = malloc((n+1)*sizeof(char *));
//                   assert(strings != NULL);
//                   // now tokenise and fill array
//                   tmp = strdup(str);
//                   char *next; int i = 0;
//                   next = strtok(tmp, sep);
//                   strings[i++] = strdup(next);
//                   while ((next = strtok(NULL,sep)) != NULL)
//                           strings[i++] = strdup(next);
//                   strings[i] = NULL;
//                   free(tmp);
//                   return strings;
//           }
