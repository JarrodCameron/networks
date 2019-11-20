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

/* The client has received payload regarding a peer to peer connection. This
 * is where the payload of the command is handled */
int ptop_handle_payload(void *payload);

/* A new connection has been created and needs to be accepted. The appropate
 * stuff to init the connectino is done here */
int ptop_accept(void);

#endif /* PTOP_H */
