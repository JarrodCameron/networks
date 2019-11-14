/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   29/10/19 22:26               *
 *                                         *
  *******************************************/

#include <stdio.h>

#include "status.h"
#include "util.h"

const char *code_to_str(enum status_code code)
{
    switch (code) {
        case init_success: return "init_success";
        case init_failed:  return "init_failed";
        case bad_uname:    return "bad_uname";
        case bad_pword:    return "bad_pword";
        case user_blocked: return "user_blocked";
        case server_error: return "server_error";
        case client_error: return "client_error";
        case comms_error:  return "comms_error";
        case already_on:   return "already_on";
        case task_success: return "task_success";
        case kill_me_now:  return "kill_me_now";
        case time_out:     return "time_out";
        case bad_command:  return "bad_command";
        case task_ready:   return "task_ready";
        case broad_logon:  return "broad_logon";
        case broad_logoff: return "broad_logoff";
        default:           return "{Unkown code}";
    }
}
