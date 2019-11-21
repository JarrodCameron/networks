#ifndef SERVER_H
#define SERVER_H

/* Return the uptime of the server for the number of seconds it has
 * been online */
time_t server_uptime(void);

/* Return the block duration as one of the arguments to the server */
time_t server_block_dur(void);

#endif /* SERVER_H */
