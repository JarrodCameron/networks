#ifndef PTOP_H
#define PTOP_H

/* This is used to handle pretty much everything regardding ptop (peer to peer)
 * connection between clients and sometimes the server */

#include <stdbool.h>

#include "config.h"

/* Defined in ptop.c */
struct ptop;

/* Return true if this is a peer to peer command, otherwise false
 * is returned */
bool ptop_is_cmd(char cmd[MAX_MSG_LENGTH]);

/* The client has entered a command that deals with peer to peer communication.
 * This is where the peer to peer request is solved. If there is an error
 * -1 is returned, otherwise 0 is returned */
int ptop_handle_cmd(char cmd[MAX_MSG_LENGTH]);

/* A new connection has been created and needs to be accepted. The appropate
 * stuff to init the connectino is done here */
int ptop_accept(void);

/* Update the read set with the appropriate file descritpors */
void ptop_fill_fd_set(fd_set *read_set);

/* Read all of the appropriate sockets from the read set and handle them
 * accordingly */
int ptop_handle_read_set(fd_set *read_set);

/* Close all of the private connections */
int ptop_stop_all(void);

#endif /* PTOP_H */
