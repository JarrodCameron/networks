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
    [init_success] = "init_success",
    [init_failed]  = "init_failed",
    [bad_uname]    = "bad_uname",
    [bad_pword]    = "bad_pword",
    [user_blocked] = "user_blocked",
    [server_error] = "server_error",
    [client_error] = "client_error",
    [comms_error]  = "comms_error",
    [already_on]   = "already_on",
    [task_success] = "task_success",
    [kill_me_now]  = "kill_me_now",
    [time_out]     = "time_out",
    [bad_command]  = "bad_command",
    [task_ready]   = "task_ready",
};


const char *code_to_str(enum status_code code)
{
    if (code < 1 || code > ARRSIZE(strings))
        return "{Unknown code}";
    return strings[code];
}
