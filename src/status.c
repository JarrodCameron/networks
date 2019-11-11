/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   29/10/19 22:26               *
 *                                         *
  *******************************************/

#include <stdio.h>

#include "status.h"
#include "util.h"

static const char *strings[] = {
    [1]  = "init_success",
    [2]  = "init_failed",
    [4]  = "bad_uname",
    [5]  = "bad_pword",
    [6]  = "user_blocked",
    [7]  = "server_error",
    [8]  = "client_error",
    [9]  = "comms_error",
    [10] = "already_on",
    [11] = "task_success",
    [12] = "kill_me_now",
    [13] = "time_out",
    [14] = "bad_command",
};


const char *code_to_str(enum status_code code)
{
    if (code < 1 || code > ARRSIZE(strings))
        return "{Unknown code}";
    return strings[code];
}
