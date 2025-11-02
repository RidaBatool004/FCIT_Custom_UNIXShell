#include "shell.h"

int handle_builtin(char **arglist) {
    if (arglist[0] == NULL)
        return 1; // Empty command handled

    // exit
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // cd
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    // help
    if (strcmp(arglist[0], "help") == 0) {
        printf("\nBuilt-in Commands:\n");
        printf("  cd <dir>   - Change current directory\n");
        printf("  help       - Show help message\n");
        printf("  exit       - Exit the shell\n");
        printf("  jobs       - Display job status (not implemented)\n\n");
        return 1;
    }

    // jobs
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0; // Not a built-in → external command
}

