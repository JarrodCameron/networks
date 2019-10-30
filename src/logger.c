/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   23/10/19 19:53               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "logger.h"

#ifndef DISABLE_LOGGING

void logs(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void elogs(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#else

void logs(const char *fmt, ...)
{
    (void) fmt;
}

void elogs(const char *fmt, ...)
{
    (void) fmt;
}

#endif /* DISABLE_LOGGING */
