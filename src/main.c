/* src/main.c */
#include "shell.h"

/* ---------- Helper functions ---------- */

static char* ltrim(char* s) {
    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
    return s;
}

static void rstrip_newline(char* s) {
    size_t len = strlen(s);
    if (len == 0) return;
    if (s[len - 1] == '\n') s[len - 1] = '\0';
}

static void rtrim_spaces(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

/* --- Feature-8 helpers --- */
static int is_assignment(const char *tok) {
    if (!tok) return 0;
    const char *eq = strchr(tok, '=');
    if (!eq) return 0;
    if (eq == tok) return 0; /* = at start invalid */
    return 1;
}

static void parse_assignment(const char *tok, char **pname, char **pvalue) {
    const char *eq = strchr(tok, '=');
    if (!eq) { *pname = NULL; *pvalue = NULL; return; }
    size_t nlen = eq - tok;
    *pname = strndup(tok, nlen);
    const char *val = eq + 1;
    if ((val[0] == '"' || val[0] == '\'') && val[strlen(val) - 1] == val[0] && strlen(val) >= 2) {
        *pvalue = strndup(val + 1, strlen(val) - 2);
    } else {
        *pvalue = strdup(val);
    }
}

/* ---------- if-then-else handler ---------- */
void handle_if_structure(char *initial_line) {
    char *cond_line = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    char *then_cmds[128];
    char *else_cmds[128];
    int then_count = 0, else_count = 0;
    int in_else = 0;

    if (initial_line && strlen(initial_line) > 0) {
        cond_line = strdup(initial_line);
    } else {
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

    while (1) {
        printf("if> ");
        read = getline(&line, &len, stdin);
        if (read <= 0) break;
        line[strcspn(line, "\n")] = '\0';

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

    int exit_code = 1;
    char **cond_args = tokenize(cond_line);
    if (cond_args && cond_args[0]) {
        int saved_out = dup(STDOUT_FILENO);
        int saved_err = dup(STDERR_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        execute_with_status(cond_args, &exit_code);

        if (saved_out != -1) { dup2(saved_out, STDOUT_FILENO); close(saved_out); }
        if (saved_err != -1) { dup2(saved_err, STDERR_FILENO); close(saved_err); }
    } else {
        fprintf(stderr, "Invalid if condition (parsing failed).\n");
        exit_code = 1;
    }

    if (cond_args) {
        for (int i = 0; cond_args[i]; ++i) free(cond_args[i]);
        free(cond_args);
    }
    free(cond_line);

    char **selected_cmds = (exit_code == 0) ? then_cmds : else_cmds;
    int selected_count = (exit_code == 0) ? then_count : else_count;

    for (int i = 0; i < selected_count; ++i) {
        char *cmd = selected_cmds[i];
        rstrip_newline(cmd);
        rtrim_spaces(cmd);
        if (strlen(cmd) == 0) continue;

        char *saveptr = NULL;
        char *piece = strtok_r(cmd, ";", &saveptr);
        while (piece) {
            char *ptrim = ltrim(piece);
            rtrim_spaces(ptrim);
            if (strlen(ptrim) > 0) {
                char **args = tokenize(ptrim);
                if (args) {
                    expand_arglist(args);
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

    for (int i = 0; i < then_count; ++i) free(then_cmds[i]);
    for (int i = 0; i < else_count; ++i) free(else_cmds[i]);
    free(line);
}

/* ---------- main shell loop ---------- */
int main() {
    char *cmdline = NULL;
    char **arglist = NULL;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        reap_terminated_jobs();

        if (cmdline[0] == '\0') { free(cmdline); continue; }

        char *trimmed = ltrim(cmdline);
        rstrip_newline(trimmed);
        rtrim_spaces(trimmed);
        if (trimmed[0] == '\0') { free(cmdline); continue; }

        if (trimmed[0] == '!') {
            int n = atoi(trimmed + 1);
            char *histcmd = get_history_command(n);
            if (histcmd) {
                free(cmdline);
                cmdline = strdup(histcmd);
                trimmed = cmdline;
            } else {
                fprintf(stderr, "No such command in history: %d\n", n);
                free(cmdline);
                continue;
            }
        }

        add_history(cmdline);
        add_to_history(cmdline);

        if (strncmp(trimmed, "if", 2) == 0 && (trimmed[2] == ' ' || trimmed[2] == '\0')) {
            char *after = trimmed + 2;
            while (*after == ' ' || *after == '\t') after++;
            handle_if_structure(after);
            free(cmdline);
            continue;
        }

        /* Support multiple commands separated by ';' */
        char *saveptr = NULL;
        char *command = strtok_r(trimmed, ";", &saveptr);
        while (command) {
            char *c = ltrim(command);
            rtrim_spaces(c);
            if (strlen(c) > 0) {
                arglist = tokenize(c);
                if (!arglist) { command = strtok_r(NULL, ";", &saveptr); continue; }

                /* --- Feature 8: Variable assignment --- */
                if (arglist[0] && is_assignment(arglist[0]) && arglist[1] == NULL) {
                    char *name = NULL, *value = NULL;
                    parse_assignment(arglist[0], &name, &value);
                    if (name) set_variable(name, value ? value : "");
                    free(name);
                    free(value);
                } else {
                    /* --- Expand variables ($VAR) before execution --- */
                    expand_arglist(arglist);

                    if (!handle_builtin(arglist)) {
                        execute(arglist);
                    }
                }

                for (int i = 0; arglist[i]; ++i) free(arglist[i]);
                free(arglist);
                arglist = NULL;
            }
            command = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
        cmdline = NULL;
    }

    free_variables();
    printf("\nShell exited.\n");
    return 0;
}

