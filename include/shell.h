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
#include <signal.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

#define HISTORY_SIZE 20   /* Custom circular buffer size for history */


/* ---- Input / Tokenization ---- */
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);

/* ---- Execution ---- */
int execute(char** arglist);

/* ---- Built-in Commands ---- */
int handle_builtin(char** arglist);

/* ---- Custom Command History ---- */
void add_to_history(const char* cmd);   /* Add command to circular buffer */
void show_history(void);                /* Print command history */
char* get_history_command(int n);       /* Retrieve nth command (1-based) */
void cleanup_history(void);             /* Free memory if dynamically allocated */

/* ---- Job Control (Feature 5 / Reaping Background Jobs) ---- */
/* ---- Job Control (Background Jobs) ---- */
#define MAX_JOBS 50

typedef struct {
    pid_t pid;
    char cmd[256];
} Job;

void add_job(pid_t pid, const char *cmd);
void reap_terminated_jobs(void);
void show_jobs(void);

#endif // SHELL_H

