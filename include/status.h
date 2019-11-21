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
    comms_error    = 8,   /* There was an error in the communicating */
    already_on     = 9,   /* The user is already logged on */
    task_success   = 10,  /* The task was done successfully */
    kill_me_now    = 11,  /* This thread should be killed */
    time_out       = 12,  /* The user has timed out, they will be logged off */
    bad_command    = 13,  /* Client sent a bad command to the server */
    task_ready     = 14,  /* The task is ready and will be done soon */
    broad_logon    = 15,  /* When the server broadcasts a user is logged on */
    broad_logoff   = 16,  /* When the server broadcasts a user is logged off */
    broad_msg      = 17,  /* General broadcast message by a user */
    dup_error      = 18,  /* There is a duplicate causing problems */
    msg_stored     = 19,  /* The message has to be stored for later use */
    client_msg     = 20,  /* Message from the client */
    backlog_msg    = 21,  /* These are messages for the backlog */
    user_unblocked = 22,  /* The user is unblocked */
    user_offline   = 23,  /* The user is offline */
};

/* Return the value of the status_code as a human readable string */
const char *code_to_str(enum status_code code);

#endif /* STATUS_H */
