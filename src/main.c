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

int main() {
    char* cmdline = NULL;
    char** arglist = NULL;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        /* skip empty allocation */
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        /* Trim leading whitespace */
        char* trimmed = ltrim(cmdline);

        /* Special case: !n (re-execution)
           Must be handled BEFORE tokenization and BEFORE adding to history.
        */
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
            char* histcmd = get_history_command(n); /* custom history lookup */
            if (histcmd == NULL) {
                fprintf(stderr, "No such command in history: %d\n", n);
                free(cmdline);
                continue;
            }
            /* Replace cmdline with a copy of histcmd */
            free(cmdline);
            cmdline = strdup(histcmd);
            if (!cmdline) {
                perror("strdup");
                continue;
            }
            /* Ensure newline trimmed */
            rstrip_newline(cmdline);
            trimmed = cmdline;
        } else {
            /* remove trailing newline if any */
            rstrip_newline(trimmed);
            /* If user typed whitespace-only, ignore */
            if (trimmed[0] == '\0') {
                free(cmdline);
                continue;
            }
        }

        /* Add to Readline history (enables arrow-key navigation) */
        add_history(cmdline);       /* readline library function */

        /* Add to custom fixed-size history buffer */
        add_to_history(cmdline);    /* your 20-entry circular buffer */

        /* Tokenize */
        arglist = tokenize(trimmed);
        if (arglist != NULL) {
            /* Check built-ins before forking */
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            /* Free tokens allocated by tokenize() */
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
            arglist = NULL;
        }

        free(cmdline);
        cmdline = NULL;
    }

    printf("\nShell exited.\n");
    return 0;
}


