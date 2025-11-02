#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

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

/* History API */
void add_history(const char* cmd);
void print_history(void);
const char* get_history(int n); /* 1-based; returns NULL if out of bounds */

#endif // SHELL_H

