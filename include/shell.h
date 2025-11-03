#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

/* History size requirement: at least last 20 commands */
#define HISTORY_SIZE 20

/* Function prototypes */
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);

/* Custom history API (renamed) */
void add_to_history(const char* cmd);         /* add to custom history buffer */
void show_history(void);                      /* print custom history */
char* get_history_command(int n);             /* 1-based */
void cleanup_history(void);

#endif // SHELL_H

