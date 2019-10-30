#ifndef LOGGER_H
#define LOGGER_H

/* The logger is responsible for displaying all messages logged for the server.
 * Since the spec says the server should print zero output all output will come
 * though the logger and not *printf. For how to enable/disable the logger see
 * "config.h"
 */

/* The regular logging command, the usage is the same as printf */
void logs(const char *fmt, ...);

/* The same as log `logf' however output is sent to stderr and not stdout */
void elogs(const char *fmt, ...);

#endif /* LOGGER_H */
