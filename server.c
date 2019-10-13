/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   13/10/19 12:33               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define check() printf("Here -> %d (%s)\n", __LINE__, __FUNCTION__)

/* Print usage and exit */
static void usage (void)
{
    fprintf(stderr, "Usage: ./server <server_port> <block_duration> <timeout>\n");
    exit(1);
}

int main (int argc, char **argv)
{

    if (argc != 4) {
        usage ();
    }

    (void) argv;

    return 0;
}

