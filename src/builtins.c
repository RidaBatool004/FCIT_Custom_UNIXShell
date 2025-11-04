#include "shell.h"

int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 1; // empty input considered handled

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
        printf("  jobs       - Display active background jobs\n");
        printf("  history    - Show recent commands\n");
        printf("  !n         - Re-execute nth command from history\n\n");
        return 1;
    }

    // jobs (updated)
    if (strcmp(arglist[0], "jobs") == 0) {
        show_jobs();   // Now uses your real job tracking
        return 1;
    }

    // history
    if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    }

    return 0; // not a builtin
}

