#ifndef STATUS_H
#define STATUS_H

/* These are general status codes that are very common through out the life of
 * the program for both the client and the server */

enum status_code {
    init_success   = 1,   /* Initialisation process was successful */
    init_failed    = 2,   /* Initialisation process failed (server error) */
    bad_uname      = 4,   /* User name is invalid */
    bad_pword      = 5,   /* Password is invalid */
    user_blocked   = 6,   /* User is blocked */
    server_error   = 7,   /* Internal server error */
    client_error   = 8,   /* Internal client error */
    comms_error    = 9,   /* There was an error in the communicating */
    already_on     = 10,  /* The user is already logged on */
    task_success   = 11,  /* The task was done successfully */
    kill_me_now    = 12,  /* This thread should be killed */
    time_out       = 13,  /* The user has timed out, they will be logged off */
    bad_command    = 14,  /* Client sent a bad command to the server */
    task_ready     = 15,  /* The task is ready and will be done soon */
    broad_logon    = 16,  /* When the server broadcasts a user is logged on */
    broad_logoff   = 17,  /* When the server broadcasts a user is logged off */
    broad_msg      = 18,  /* General broadcast message by a user */
    dup_error      = 19,  /* There is a duplicate causing problems */
    msg_stored     = 20,  /* The message has to be stored for later use */
    code_true      = 21,  /* The equivalent of "true" */
    code_false     = 22,  /* The equivalent of "false" */
    client_msg     = 23,  /* Message from the client */
    backlog_msg    = 24,  /* These are messages for the backlog */
    user_unblocked = 25,  /* The user is unblocked */
    user_offline   = 26,  /* The user is offline */
};

/* Return the value of the status_code as a human readable string */
const char *code_to_str(enum status_code code);

#endif /* STATUS_H */
