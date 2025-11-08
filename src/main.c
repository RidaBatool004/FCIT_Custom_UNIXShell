/* src/main.c */
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

/*
 * handle_if_structure:
 *   initial_line: the string after the "if" token on the same input line.
 *                 may be empty (""), in which case we prompt for the condition.
 */
void handle_if_structure(char *initial_line) {
    char *cond_line = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    char *then_cmds[128];
    char *else_cmds[128];
    int then_count = 0, else_count = 0;
    int in_else = 0;

    /* 1) Determine the condition line: either from initial_line or prompt */
    if (initial_line && strlen(initial_line) > 0) {
        cond_line = strdup(initial_line);
    } else {
        /* prompt for condition */
        printf("if> ");
        read = getline(&line, &len, stdin);
        if (read <= 0) { free(line); return; }
        line[strcspn(line, "\n")] = '\0';
        cond_line = strdup(line);
    }

    if (!cond_line) { free(line); return; }
    rstrip_newline(cond_line);
    rtrim_spaces(cond_line);

    if (strlen(cond_line) == 0) {
        fprintf(stderr, "Invalid if condition.\n");
        free(cond_line);
        free(line);
        return;
    }

    /* 2) Read the then/else/fi block lines */
    while (1) {
        printf("if> ");
        read = getline(&line, &len, stdin);
        if (read <= 0) break;
        line[strcspn(line, "\n")] = '\0';

        /* skip empty lines */
        char *trim = ltrim(line);
        rtrim_spaces(trim);
        if (strlen(trim) == 0) continue;

        if (strcmp(trim, "then") == 0) {
            continue;
        } else if (strcmp(trim, "else") == 0) {
            in_else = 1;
            continue;
        } else if (strcmp(trim, "fi") == 0) {
            break;
        } else {
            if (!in_else) then_cmds[then_count++] = strdup(trim);
            else else_cmds[else_count++] = strdup(trim);
        }
    }
    then_cmds[then_count] = NULL;
    else_cmds[else_count] = NULL;

    /* 3) Evaluate condition quietly (suppress its stdout/stderr) using execute_with_status */
    int exit_code = 1;
    char **cond_args = tokenize(cond_line);
    if (cond_args && cond_args[0]) {
        /* Save stdout/stderr */
        int saved_out = dup(STDOUT_FILENO);
        int saved_err = dup(STDERR_FILENO);

        /* Redirect both to /dev/null */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        /* execute_with_status should set exit_code appropriately */
        execute_with_status(cond_args, &exit_code);

        /* Restore stdout/stderr */
        if (saved_out != -1) { dup2(saved_out, STDOUT_FILENO); close(saved_out); }
        if (saved_err != -1) { dup2(saved_err, STDERR_FILENO); close(saved_err); }
    } else {
        fprintf(stderr, "Invalid if condition (parsing failed).\n");
        exit_code = 1;
    }

    /* free cond_args */
    if (cond_args) {
        for (int i = 0; cond_args[i]; ++i) free(cond_args[i]);
        free(cond_args);
    }
    free(cond_line);

    /* 4) Execute selected block */
    char **selected_cmds = (exit_code == 0) ? then_cmds : else_cmds;
    int selected_count = (exit_code == 0) ? then_count : else_count;

    for (int i = 0; i < selected_count; ++i) {
        char *cmd = selected_cmds[i];
        rstrip_newline(cmd);
        rtrim_spaces(cmd);
        if (strlen(cmd) == 0) continue;

        /* support chaining in a line inside the block (e.g., "echo a ; echo b") */
        char *saveptr = NULL;
        char *piece = strtok_r(cmd, ";", &saveptr);
        while (piece) {
            char *ptrim = ltrim(piece);
            rtrim_spaces(ptrim);
            if (strlen(ptrim) > 0) {
                char **args = tokenize(ptrim);
                if (args) {
                    if (!handle_builtin(args)) {
                        execute(args);
                    }
                    for (int j = 0; args[j]; ++j) free(args[j]);
                    free(args);
                }
            }
            piece = strtok_r(NULL, ";", &saveptr);
        }
    }

    /* cleanup */
    for (int i = 0; i < then_count; ++i) free(then_cmds[i]);
    for (int i = 0; i < else_count; ++i) free(else_cmds[i]);
    free(line);
}
 
int main() {
    char* cmdline = NULL;
    char** arglist = NULL;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        reap_terminated_jobs();

        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        char* trimmed = ltrim(cmdline);
        rstrip_newline(trimmed);
        rtrim_spaces(trimmed);
        if (trimmed[0] == '\0') { free(cmdline); continue; }

        /* Handle !n BEFORE tokenizing */
        if (trimmed[0] == '!') {
            int n = atoi(trimmed + 1);
            char* histcmd = get_history_command(n);
            if (histcmd) {
                free(cmdline);
                cmdline = strdup(histcmd);
                if (!cmdline) { perror("strdup"); continue; }
                trimmed = cmdline;
            } else {
                fprintf(stderr, "No such command in history: %d\n", n);
                free(cmdline);
                continue;
            }
        }

        /* Add to readline + custom history */
        add_history(cmdline);
        add_to_history(cmdline);

        /* If-block handling: pass the rest of the line after "if" as initial condition */
        if (strncmp(trimmed, "if", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            /* capture rest after 'if' */
            char *after = trimmed + 2;
            while (*after == ' ' || *after == '\t') after++;
            /* call handler with the rest (may be empty) */
            handle_if_structure(after);
            free(cmdline);
            continue;
        }

        /* Support multiple commands separated by ';' on a single line */
        char *saveptr = NULL;
        char *command = strtok_r(trimmed, ";", &saveptr);
        while (command) {
            char *c = ltrim(command);
            rtrim_spaces(c);
            if (strlen(c) > 0) {
                arglist = tokenize(c);
                if (arglist) {
                    if (!handle_builtin(arglist))
                        execute(arglist);
                    for (int i = 0; arglist[i]; ++i) free(arglist[i]);
                    free(arglist);
                    arglist = NULL;
                }
            }
            command = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
        cmdline = NULL;
    }

    printf("\nShell exited.\n");
    return 0;
}

