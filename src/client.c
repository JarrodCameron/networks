/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:35               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"

/* Print usage and exit */
static void usage (void)
{
    fprintf (stderr, "Usage: ./client <server_ip> <server_port>\n");
    exit (1);
}

int main (int argc, char **argv)
{

    if (argc != 3) {
        usage ();
    }

    (void) argv;

    return 0;
}

