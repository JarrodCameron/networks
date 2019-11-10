// mysh.c ... a small shell
// Started by John Shepherd, September 2018
// Completed by Jarrod Cameron (z5210220), September/October 2018

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>
#include <assert.h>
#include <fcntl.h>
#include "history.h"

/////////////////
// stdin  -> 0 //
// stdout -> 1 //
// stderr -> 2 //
/////////////////

#define TRUE             1
#define FALSE            0

// Many functions will have a return status
//   which will be either of these two:
#define VALID_COMMAND    0
#define INVALID_COMMAND  1

// The four possible shell built in commands
#define SHELL_NONE       2
#define SHELL_EXIT       3
#define SHELL_HIST       4
#define SHELL_PWD        5
#define SHELL_CD         6

// All types of redirection
#define REDIR_NONE       7
#define REDIR_OUTPUT     8
#define REDIR_INPUT      9

#define check() printf("Here -> %d\n", __LINE__)

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(char *);

struct command {
        char *line;
        char **args;
        char *filename;
        char *redir_file;
        int redir_type;
        int retval;
        int shell_cmd;
};

// Function forward references

void trim(char *);
int strContains(char *, char *);
char **tokenise(char *, char *);
void freeTokens(char **);
char *findExecutable(char *, char **);
int isExecutable(char *);
void prompt(void);
char **get_path (char **envp);

// Global Constants

#define MAXLINE 200

// Global Data

// Helper functions
int init_command (struct command *cmd, char line[]);
int apply_substitution (struct command *cmd);
void free_cmd (struct command *cmd);
int tokenise_cmd (struct command *cmd);
int apply_expansion (struct command *cmd);
glob_t globify (char **tokens);
char **dup_tokens (char **tokens);
int n_tokens (char **tokens);
void check_shell_cmds (struct command *cmd);
int apply_shell_cmd (struct command *cmd);
int exec_shell_hist (void);
int exec_shell_pwd (void);
int exec_shell_cd (struct command *cmd);
int check_redirection (struct command *cmd);
int apply_redirection (struct command *cmd);
int get_filename (struct command *cmd, char **path);
int redirect_output (struct command *cmd);
int redirect_input (struct command *cmd);
int exec_command (struct command *cmd, char **path);
int get_retval (struct command *cmd, pid_t pid);
void show_retval (struct command *cmd);
int is_empmty (char *line);

// Main program
// Set up enviroment and then run main loop
// - read command, execute command, repeat

int main(int argc, char *argv[], char *envp[]) {
        // Keep gcc happy when using the `-Wextra` flag
        (void) argc;
        (void) argv;

        char **path = get_path (envp);

        initCommandHistory();

        char line[MAXLINE];
        prompt();
        while (fgets(line, MAXLINE, stdin) != NULL) {

                if (is_empmty (line)) {
                        prompt ();
                        continue;
                } else {
                        // remove leading/trailing space
                        trim (line);
                }

                // This will contain all of the information about
                //   the command to be executed
                struct command command;

                // Fills: `line`
                if (init_command (&command, line) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // If command needs to be substituted then apply the valid substitution
                if (apply_substitution (&command) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // Fills: `args`
                if (tokenise_cmd (&command) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // Apply expansion to all arguments
                // i.e.  ('*', '?', '[', '~')
                if (apply_expansion (&command) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // Fills: `shell_cmd`
                check_shell_cmds (&command);

                int dummy = apply_shell_cmd (&command);
                if (dummy == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                } else if (dummy == SHELL_EXIT) {
                        addToCommandHistory (command.line);
                        free_cmd (&command);
                        break;
                } else if (dummy != SHELL_NONE) {
                        // If the current iteration is still going then
                        //  the command is correct and should be saved
                        addToCommandHistory (command.line);

                        // `h`, `pwd`, and `cd` don't need to do anything else
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // Fills: `redir_file`, `redir_type`
                if (check_redirection (&command) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                // Fills: `filename` with the full path to the file
                if (get_filename (&command, path) == INVALID_COMMAND) {
                        free_cmd (&command);
                        prompt ();
                        continue;
                }

                printf ("Running %s ...\n", command.filename);
                printf ("--------------------\n");
                pid_t pid = fork ();
                assert (pid >= 0);

                if (pid == 0) {
                        // child process
                        if (apply_redirection (&command) == INVALID_COMMAND)
                                exit (INVALID_COMMAND);
                        exec_command (&command, envp);
                } else {
                        // parent process
                        get_retval (&command, pid);
                }

                printf ("--------------------\n");
                show_retval (&command);

                // Cleanup
                addToCommandHistory (command.line);
                free_cmd (&command);
                prompt();
        }
        saveCommandHistory();
        cleanCommandHistory();
        freeTokens (path);
        printf("\n");
        return(EXIT_SUCCESS);
}

// findExecutable: look for executable in PATH
char *findExecutable(char *cmd, char **path) {
        char executable[MAXLINE];
        executable[0] = '\0';
        if (cmd[0] == '/' || cmd[0] == '.') {
                strcpy(executable, cmd);
                if (!isExecutable(executable))
                        executable[0] = '\0';
        }
        else {
                int i;
                for (i = 0; path[i] != NULL; i++) {
                        sprintf(executable, "%s/%s", path[i], cmd);
                        if (isExecutable(executable)) break;
                }
                if (path[i] == NULL) executable[0] = '\0';
        }
        if (executable[0] == '\0')
                return NULL;
        else
                return strdup(executable);
}

// isExecutable: check whether this process can execute a file
int isExecutable(char *cmd) {
        struct stat s;
        // must be accessible
        if (stat(cmd, &s) < 0)
                return 0;
        // must be a regular file
        //if (!(s.st_mode & S_IFREG))
        if (!S_ISREG(s.st_mode))
                return 0;
        // if it's owner executable by us, ok
        if (s.st_uid == getuid() && s.st_mode & S_IXUSR)
                return 1;
        // if it's group executable by us, ok
        if (s.st_gid == getgid() && s.st_mode & S_IXGRP)
                return 1;
        // if it's other executable by us, ok
        if (s.st_mode & S_IXOTH)
                return 1;
        return 0;
}

// tokenise: split a string around a set of separators
// create an array of separate strings
// final array element contains NULL
char **tokenise(char *str, char *sep) {
        // temp copy of string, because strtok() mangles it
        char *tmp;
        // count tokens
        tmp = strdup(str);
        int n = 0;
        strtok(tmp, sep); n++;
        while (strtok(NULL, sep) != NULL) n++;
        free(tmp);
        // allocate array for argv strings
        char **strings = malloc((n+1)*sizeof(char *));
        assert(strings != NULL);
        // now tokenise and fill array
        tmp = strdup(str);
        char *next; int i = 0;
        next = strtok(tmp, sep);
        strings[i++] = strdup(next);
        while ((next = strtok(NULL,sep)) != NULL)
                strings[i++] = strdup(next);
        strings[i] = NULL;
        free(tmp);
        return strings;
}

// freeTokens: free memory associated with array of tokens
void freeTokens(char **toks) {
        for (int i = 0; toks[i] != NULL; i++)
                free(toks[i]);
        free(toks);
}

// trim: remove leading/trailing spaces from a string
void trim(char *str) {
        int first, last;
        first = 0;
        while (isspace(str[first])) first++;
        last  = strlen(str)-1;
        while (isspace(str[last])) last--;
        int i, j = 0;
        for (i = first; i <= last; i++) str[j++] = str[i];
        str[j] = '\0';
}

// strContains: does the first string contain any char from 2nd string?
int strContains(char *str, char *chars) {
        for (char *s = str; *s != '\0'; s++) {
                for (char *c = chars; *c != '\0'; c++) {
                        if (*s == *c) return 1;
                }
        }
        return 0;
}

// prompt: print a shell prompt
// done as a function to allow switching to $PS1
void prompt(void) {
        printf("mymysh$ ");
}

// Returns the tokenised path variable form envp
char **get_path (char **envp) {
        int i;       // generic index
        char **path;

        // set up command PATH from environment variable
        for (i = 0; envp[i] != NULL; i++) {
                if (strncmp(envp[i], "PATH=", 5) == 0) break;
        }
        if (envp[i] == NULL)
                path = tokenise("/bin:/usr/bin",":");
        else
                // &envp[i][5] skips over "PATH=" prefix
                path = tokenise(&envp[i][5],":");
#ifdef DBUG
        for (i = 0; path[i] != NULL;i++)
                printf("path[%d] = %s\n",i,path[i]);
#endif
        return path;
}

// Insert a struct at the location `cmd`
// cmd->line should contain a copy of the line
int init_command (struct command *cmd, char line[]) {

        // Empty command
        if (line[0] == '\n') {
                return INVALID_COMMAND;
        }

        *cmd = (struct command) {
                .line = NULL,
                .args = NULL,
                .filename = NULL,
                .redir_file = NULL,
                .redir_type = REDIR_NONE,
                .retval = 0, // Doesn't matter
                .shell_cmd = SHELL_NONE
        };

        cmd->line = strdup (line);
        assert (cmd->line);

        return VALID_COMMAND;
}

// Free all malloc'd items from memory
// NOTE: All pointer point to items on the heap or NULL
//         therefore freeing is always fine
void free_cmd (struct command *cmd) {
        free (cmd->line);
        free (cmd->filename);
        free (cmd->redir_file);
        if (cmd->args != NULL) {
                for (int i = 0; cmd->args[i] != NULL; i++) {
                        free (cmd->args[i]);
                }
                free (cmd->args);
        }
}

// Checking if `cmd->line[0] == '!'` and `cmd->line[1] == '!'`
//   is poor design however this is how Jas's solution is implemented.
//   This is seen when `!!` and `!!!!!!!!!` (or even `!!123`) give the output
int apply_substitution (struct command *cmd) {
        if (strcmp (cmd->line, "!") == 0) {
                fprintf (stderr, "Invalid history substitution\n");
                return INVALID_COMMAND;
        } else if (cmd->line[0] != '!') {
                return VALID_COMMAND;
        }

        int n;
        if (cmd->line[1] == '!') {
                n = LAST_SUB;
        } else if (sscanf (cmd->line, "!%d", &n) != 1) {
                return INVALID_COMMAND;
        } else if (n < 0) {
                fprintf (stderr, "Invalid history substitution\n");
                return INVALID_COMMAND;
        }

        free (cmd->line);
        cmd->line = getCommandFromHistory (n);

        if (cmd->line == NULL) {
                return INVALID_COMMAND;
        } else {
                return VALID_COMMAND;
        }
}

// I would like to thank Jas for doing 99% of this function
int tokenise_cmd (struct command *cmd) {
        cmd->args = tokenise (cmd->line, " ");
        if (cmd->args == NULL) {
                return INVALID_COMMAND;
        }
        return VALID_COMMAND;
}

// The args haven't been expanded yet
// The args will be expaneded
// That is about it...
int apply_expansion (struct command *cmd) {
        glob_t g = globify (cmd->args);
        freeTokens (cmd->args);
        cmd->args = dup_tokens (g.gl_pathv);
        globfree (&g);
        return VALID_COMMAND;
}

// Given a char** apply glob to all of the necessary strings
// NOTE: If you use GLOB_APPEND the first time glob is used then the program will
//   get a segmentation fault
glob_t globify (char **tokens) {

        glob_t g;
        int ret = glob (tokens[0], GLOB_NOCHECK | GLOB_TILDE, NULL, &g);
        assert (ret == 0);
        for (int i = 1; tokens[i] != NULL; i++) {
                ret = glob (tokens[i], GLOB_NOCHECK | GLOB_TILDE | GLOB_APPEND, NULL, &g);
                assert (ret == 0);
        }
        return g;
}

// Duplicates the list of tokens which is NULL-terminated
char **dup_tokens (char **tokens) {
        int n = n_tokens (tokens);
        char **ret = malloc ((n+1) * sizeof (char *));
        int i;
        for (i = 0; tokens[i] != NULL; i++) {
                ret[i] = strdup (tokens[i]);
                assert (ret[i] != NULL);
        }
        ret[i] = NULL;
        return ret;
}

// Returns the number of tokens is a NULL-terminated array
int n_tokens (char **tokens) {
        int count = 0;
        while (tokens[count] != NULL) {
                count++;
        }
        return count;
}

// Checks the first argument of the command and sets the appropriate flag
// Things to note:
//   - There are two variations for history; `h` and `history`
//   - This does not check if `cd` has a second argument
//   - If the user follows most of these with an useless second argument the
//       second argument is ignored. For example `pwd AAA` is the same as `pwd`
//       as per Jas's solution
void check_shell_cmds (struct command *cmd) {
        if (strcmp (cmd->args[0], "exit") == 0) {
                cmd->shell_cmd = SHELL_EXIT;
        } else if (strcmp (cmd->args[0], "h") == 0) {
                cmd->shell_cmd = SHELL_HIST;
        } else if (strcmp (cmd->args[0], "history") == 0) {
                cmd->shell_cmd = SHELL_HIST;
        } else if (strcmp (cmd->args[0], "pwd") == 0) {
                cmd->shell_cmd = SHELL_PWD;
        } else if (strcmp (cmd->args[0], "cd") == 0) {
                cmd->shell_cmd = SHELL_CD;
        } else {
                cmd->shell_cmd = SHELL_NONE;
        }
}

// Apply the given shell command
// The loop can't terminate inside of this function so the calling function
//   must exit this is indicated by the `return SHELL_EXIT`
int apply_shell_cmd (struct command *cmd) {
        if (cmd->shell_cmd == SHELL_EXIT) {
                return SHELL_EXIT;
        } else if (cmd->shell_cmd == SHELL_HIST) {
                return exec_shell_hist ();
        } else if (cmd->shell_cmd == SHELL_PWD) {
                return exec_shell_pwd ();
        } else if (cmd->shell_cmd == SHELL_CD) {
                return exec_shell_cd (cmd);
        } else { // cmd->shell_cmd == SHELL_NONE
                return SHELL_NONE;
        }
}

// Literally impossible to screw up...
int exec_shell_hist (void) {
        showCommandHistory (stdout);
        return VALID_COMMAND;
}

// Prints the current working directory
int exec_shell_pwd (void) {
        char buf[MAXLINE];
        assert (getcwd (buf, MAXLINE) != NULL);
        printf ("%s\n", buf);
        return VALID_COMMAND;
}

// Changes directories to the directory specified.
// If no directory is specified then move to the current directory.
int exec_shell_cd (struct command *cmd) {
        char *path = cmd->args[1];
        if (cmd->args[1] == NULL)
                path = ".";

        glob_t g;
        glob (path, GLOB_NOCHECK | GLOB_TILDE | GLOB_ONLYDIR, NULL, &g);

        if (chdir (g.gl_pathv[0]) != 0) {
                perror (cmd->args[1]);
                globfree (&g);
                return INVALID_COMMAND;
        }

        // Print current directory after moving
        exec_shell_pwd ();

        globfree (&g);
        return VALID_COMMAND;
}

// Fills `cmd` with information about the redirection of the input or output
// NOTE: `mymysh` can't redirected input AND output
int check_redirection (struct command *cmd) {
        cmd->redir_type = REDIR_NONE;
        int n_in = 0;
        int n_out = 0;
        int index = -1;
        int i;
        for (i = 0; cmd->args[i] != NULL; i++) {
                if (strcmp (cmd->args[i], "<") == 0) {
                        n_in++;
                        index = i;
                } else if (strcmp (cmd->args[i], ">") == 0) {
                        n_out++;
                        index = i;
                }
        }
        if (n_in == 0 && n_out == 0)
                // No redirection
                return VALID_COMMAND;

        // The number of file to redirect to/from
        int n_redirs = n_tokens (&(cmd->args[index+1]));

        if (n_in + n_out > 1 || index == 0 || n_redirs != 1) {
                fprintf (stderr, "Invalid i/o redirection\n");
                return INVALID_COMMAND;
        }

        if (n_in == 1) {
                cmd->redir_type = REDIR_INPUT;
        } else {
                cmd->redir_type = REDIR_OUTPUT;
        }

        free (cmd->args[index]);
        cmd->args[index] = NULL;
        cmd->redir_file = cmd->args[index+1];

        return VALID_COMMAND;
}

// Returns the full path of an executable
int get_filename (struct command *cmd, char **path) {
        cmd->filename = findExecutable (cmd->args[0], path);
        if (cmd->filename == NULL) {
                fprintf (stderr, "%s: Command not found\n", cmd->args[0]);
                return INVALID_COMMAND;
        } else {
                return VALID_COMMAND;
        }
}

// Apply the given redirection specified in the `cmd`
// NOTE: This function is called by the child process
int apply_redirection (struct command *cmd) {
        if (cmd->redir_type == REDIR_OUTPUT) {
                return redirect_output (cmd);
        } else if (cmd->redir_type == REDIR_INPUT) {
                return redirect_input (cmd);
        } else { // cmd->redir_type == REDIR_NONE
                return VALID_COMMAND;
        }
}

// Redirect the output
int redirect_output (struct command *cmd) {
        int fd = open (cmd->redir_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if (fd < 0) {
                perror (cmd->redir_file);
                return INVALID_COMMAND;
        }
        close (1); // close stdout
        dup2 (fd, 1);
        return VALID_COMMAND;
}

// Redirect the input
int redirect_input (struct command *cmd) {
        int fd = open (cmd->redir_file, O_RDONLY);
        if (fd < 0) {
                perror ("Input redirection");
                return INVALID_COMMAND;
        }
        close (0); // close stdin
        dup2 (fd, 0);
        return VALID_COMMAND;
}

// Calling `execve` execute the new command
int exec_command (struct command *cmd, char **envp) {
        int ret = execve (cmd->filename, cmd->args, envp);

        // If the process is still running after the `execve' call
        //   there was an error
        fprintf (stderr, "%s: unknown type of executable\n", cmd->line);
        exit (ret);
}

// Return the exit status of the child process
int get_retval (struct command *cmd, pid_t pid) {
        int status;
        waitpid (pid, &status, 0);
        cmd->retval = WEXITSTATUS (status);
        return cmd->retval;
}

// Prints the return value of the hild process to the scree
void show_retval (struct command *cmd) {
        printf ("Returns %d\n", cmd->retval);
}

// A line is empty iff it only contains spaces and is NULL terminated and
//   contains a single newline char
// Majority of the time the string will end with a newline followed
//   by a NULL terminated byte.
// If the number of charcters is exactly one less than the maximium buffer
//   size the string will be NULL terminated and the next buffer will only
//   contain a `\n\0`
int is_empmty (char *line) {
        char *c = line;
        while (*c != '\0') {
                if (*c != ' ' && *c != '\n') {
                        return FALSE;
                }
                c++;
        }
        return TRUE;
}

====================
 history.c
====================

// COMP1521 18s2 mysh ... command history
// Implements an abstract data object

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>
#include "history.h"

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(const char *s);

// Command History
// array of command lines
// each is associated with a sequence number

#define MAXHIST 20
#define MAXSTR  200

#define HIST_FILE      ".mymysh_history"

#define check() printf("Here -> %d\n", __LINE__)

// I hate typedefs...
struct cmd_line {
        int cmd_num;
        char *line;
        struct cmd_line *next;
        struct cmd_line *prev;
};

struct _history_list {
        int n_lines;
        int n_commands;
        char *hist_file_path;
        struct cmd_line *first;
        struct cmd_line *last;
};

static struct _history_list history_list;

static void fill_history (void);
static void assert_file (char *path);
static char *get_hist_file_path (void);
static void append_cmd_line (const char *buf);
static struct cmd_line *get_new_line (const char *buf);
static void free_line (struct cmd_line *cmd);

// initCommandHistory()
// - initialise the data structure
// - read from .history if it exists
int initCommandHistory (void) {
        history_list.n_lines = 0;
        history_list.n_commands = 1;
        history_list.first = NULL;
        history_list.last = NULL;
        fill_history ();
        return history_list.n_lines;
}

// addToCommandHistory()
// - add a command line to the history list
// - overwrite oldest entry if buffer is full
void addToCommandHistory (const char *line) {
        append_cmd_line (line);
}

// showCommandHistory()
// - display the list of
void showCommandHistory (FILE *out) {
        struct cmd_line *curr = history_list.first;
        while (curr != NULL) {
                fprintf (out, " %3d  %s\n", curr->cmd_num, curr->line);
                curr = curr->next;
        }
}

// getCommandFromHistory()
// - get the command line for specified command
// - returns NULL if no command with this number
char *getCommandFromHistory (int nth_cmd) {


        struct cmd_line *curr;

        if (nth_cmd == LAST_SUB) {
                curr = history_list.last;
        } else {
                curr = history_list.first;
                while (curr != NULL && curr->cmd_num != nth_cmd) {
                        curr = curr->next;
                }
        }

        char *ret ;
        if (curr != NULL) {
                ret = strdup (curr->line);
                assert (ret);
        } else {
                ret = NULL;
                fprintf (stderr, "No command #%d\n", nth_cmd);
        }
        return ret;
}

// saveCommandHistory()
// - write history to $HOME/.mymysh_history
void saveCommandHistory (void) {
        char *path = history_list.hist_file_path;
        struct cmd_line *curr = history_list.first;
        FILE *hist_stream = fopen (path, "w");
        while (curr != NULL) {
                fprintf (hist_stream, " %3d  %s\n", curr->cmd_num, curr->line);
                curr = curr->next;
        }
        fclose (hist_stream);
}

// cleanCommandHistory
// - release all data allocated to command history
void cleanCommandHistory (void) {
        struct cmd_line *curr = history_list.first;
        while (curr != NULL) {
                struct cmd_line *next = curr->next;
                free (curr->line);
                free (curr);
                curr = next;
        }
        history_list.n_lines = 0;
        free (history_list.hist_file_path);
        history_list.first = NULL;
        history_list.last = NULL;
}

// If the file does not exist then make one
// If we don't have permissions to open the file then exit
// Else do nothing
// NOTE: `0600' is the minimum requirements to operate on a file
static void assert_file (char *path) {
        int fd = open (path, O_CREAT, 0600);
        if (fd < 0) {
                err (errno, "ERROR: can't access \"%s\"\n", path);
        }
        close (fd);
}

// Returns the absolute file location of the HIST_FILE
// `ret' is malloc'd and therefore needs to be freed
static char *get_hist_file_path (void) {
        // This should return the absolute path of the history file
        glob_t g;
        glob ("~/", GLOB_NOCHECK | GLOB_TILDE, NULL, &g);
        int len = strlen (g.gl_pathv[0]) + strlen (HIST_FILE) + 1;
        char *ret = malloc (len * sizeof (char));
        assert (ret);
        strcpy (ret, g.gl_pathv[0]);
        strcat (ret, HIST_FILE);
        globfree (&g);
        return ret;
}

// Frees a given line off the heap
static void free_line (struct cmd_line *cmd) {
        free (cmd->line);
        free (cmd);
}

// Read the HIST_FILE and insert records into the `history_list'
static void fill_history (void) {

        history_list.hist_file_path = get_hist_file_path ();
        assert_file (history_list.hist_file_path);
        FILE *hist_stream = fopen (history_list.hist_file_path, "r");
        char buffer[MAXSTR];
        int num;
        while (fscanf (hist_stream, " %3d  %[^\n]", &num, buffer) != EOF) {
                history_list.n_commands = num;
                append_cmd_line (buffer);
        }
        fclose (hist_stream);
}

// Returns a fresh cmd_line off the heap
static struct cmd_line *get_new_line (const char *buf) {
        struct cmd_line *ret = malloc (sizeof (struct cmd_line));
        ret->next = NULL;
        ret->prev = NULL;

        ret->line = strdup (buf);
        assert (ret->line);

        ret->cmd_num = history_list.n_commands;
        history_list.n_commands += 1;

        return ret;
}

// The buffer might be given to us off the stack therefore we will
//   need to create a new one
// There are three casses:
//   1) This is the first element in the list
//   2) The number of elements is less than MAXHIST
//   3) The number of elements in list is MAXHIST
static void append_cmd_line (const char *buf) {
        struct cmd_line *new_line = get_new_line (buf);

        if (history_list.n_lines == 0) {
                history_list.first = new_line;
                history_list.last = new_line;
                history_list.n_lines += 1;
        } else if (history_list.n_lines < MAXHIST) {
                new_line->prev = history_list.last;
                history_list.last->next = new_line;
                history_list.last = new_line;
                history_list.n_lines += 1;
        } else {
                // Remove first
                history_list.first = history_list.first->next;
                free_line (history_list.first->prev);
                history_list.first->prev = NULL;

                // Add last
                new_line->prev = history_list.last;
                history_list.last->next = new_line;
                history_list.last = new_line;
        }
}
