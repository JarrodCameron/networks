/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   30/10/19 19:51               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "banner.h"

void banner_logged_in(void)
{
    printf("/----------------------------\\\n");
    printf("| Welcome! You are logged in |\n");
    printf("\\----------------------------/\n");
}

