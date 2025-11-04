/* src/shell.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"

// built-in commands for completion
const char* builtins[] = {"cd", "help", "exit", "history", "jobs", NULL};

char* command_generator(const char* text, int state) {
    static int list_index;
    const char* name;

    if (!state)
        list_index = 0;

    while ((name = builtins[list_index++])) {
        if (strncmp(name, text, strlen(text)) == 0)
            return strdup(name);
    }
    return NULL;
}

char** my_completion(const char* text, int start, int end) {
    (void)start; (void)end;
    // Only complete the first word (command)
    if (start == 0)
        return rl_completion_matches(text, command_generator);
    else
        return NULL;
}

char* read_cmd(char* prompt, FILE* fp) {
    (void)fp;
    rl_attempted_completion_function = my_completion;

    char* cmdline = readline(prompt);  // Readline handles editing & arrow keys
    if (!cmdline) {
        printf("\n");
        return NULL;
    }

    // DO NOT call a custom add_history here (that would collide with Readline symbol).
    // We'll add to Readline history and custom history in main(), once the command is validated.

    return cmdline;
}

/* Tokenizer supporting <, >, |, ;, and & */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || *cmdline == '\0') return NULL;

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = calloc(ARGLEN, sizeof(char));
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;

        /* treat <, >, |, ;, & as separate tokens */
        if (*cp == '<' || *cp == '>' || *cp == '|' || *cp == ';' || *cp == '&') {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            argnum++;
            cp++;
            continue;
        }

        /* normal word token */
        int len = 0;
        while (*cp != '\0' && *cp != ' ' && *cp != '\t' &&
               *cp != '<' && *cp != '>' && *cp != '|' &&
               *cp != ';' && *cp != '&') {
            arglist[argnum][len++] = *cp++;
        }
        arglist[argnum][len] = '\0';
        argnum++;
    }

    arglist[argnum] = NULL;
    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }
    return arglist;
}


