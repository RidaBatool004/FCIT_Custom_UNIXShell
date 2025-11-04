#include "shell.h"

/* Helper: trim leading spaces */
static char* ltrim(char* s) {
    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
    return s;
}

/* Helper: trim trailing newline */
static void rstrip_newline(char* s) {
    size_t len = strlen(s);
    if (len == 0) return;
    if (s[len - 1] == '\n') s[len - 1] = '\0';
}

/* Helper: trim trailing spaces */
static void rtrim_spaces(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

int main() {
    char* cmdline = NULL;
    char** arglist = NULL;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        /* Reap any finished background jobs */
        reap_terminated_jobs();

        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        char* trimmed = ltrim(cmdline);

        /* Handle history re-execution (!n) BEFORE tokenizing */
        if (trimmed[0] == '!') {
            char *numstr = trimmed + 1;
            if (*numstr == '\0') {
                fprintf(stderr, "Usage: !n  (n is command number)\n");
                free(cmdline);
                continue;
            }
            int n = atoi(numstr);
            if (n <= 0) {
                fprintf(stderr, "Invalid history reference: %s\n", numstr);
                free(cmdline);
                continue;
            }
            char* histcmd = get_history_command(n);
            if (histcmd == NULL) {
                fprintf(stderr, "No such command in history: %d\n", n);
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(histcmd);
            if (!cmdline) {
                perror("strdup");
                continue;
            }
            rstrip_newline(cmdline);
            trimmed = cmdline;
        } else {
            rstrip_newline(trimmed);
            rtrim_spaces(trimmed);
            if (trimmed[0] == '\0') {
                free(cmdline);
                continue;
            }
        }

        /* Add to both readline and custom history */
        add_history(cmdline);
        add_to_history(cmdline);

        /* NEW: handle multiple commands separated by ';' */
        char* command = strtok(trimmed, ";");
        while (command != NULL) {
            command = ltrim(command);
            rtrim_spaces(command);

            if (strlen(command) > 0) {
                arglist = tokenize(command);
                if (arglist != NULL) {
                    if (!handle_builtin(arglist)) {
                        execute(arglist);
                    }

                    /* Free tokenized args */
                    for (int i = 0; arglist[i] != NULL; i++)
                        free(arglist[i]);
                    free(arglist);
                    arglist = NULL;
                }
            }

            command = strtok(NULL, ";");
        }

        free(cmdline);
        cmdline = NULL;
    }

    printf("\nShell exited.\n");
    return 0;
}



