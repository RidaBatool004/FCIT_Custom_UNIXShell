#include "shell.h"

int execute(char* arglist[]) {
    int in_redirect = -1, out_redirect = -1, pipe_pos = -1;

    // Identify operators
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) in_redirect = i;
        else if (strcmp(arglist[i], ">") == 0) out_redirect = i;
        else if (strcmp(arglist[i], "|") == 0) pipe_pos = i;
    }

    // PIPE HANDLING
    if (pipe_pos != -1) {
        arglist[pipe_pos] = NULL;  // split commands
        char **left_cmd = arglist;
        char **right_cmd = &arglist[pipe_pos + 1];

        int fds[2];
        if (pipe(fds) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t left_pid = fork();
        if (left_pid == 0) {
            close(fds[0]);                // close read end
            dup2(fds[1], STDOUT_FILENO);  // redirect stdout to pipe
            close(fds[1]);
            execvp(left_cmd[0], left_cmd);
            perror("execvp left");
            exit(1);
        }

        pid_t right_pid = fork();
        if (right_pid == 0) {
            close(fds[1]);                // close write end
            dup2(fds[0], STDIN_FILENO);   // redirect stdin from pipe
            close(fds[0]);
            execvp(right_cmd[0], right_cmd);
            perror("execvp right");
            exit(1);
        }

        close(fds[0]);
        close(fds[1]);
        waitpid(left_pid, NULL, 0);
        waitpid(right_pid, NULL, 0);
        return 0;
    }

    // REDIRECTION HANDLING
    int in_fd = -1, out_fd = -1;
    if (in_redirect != -1) {
        in_fd = open(arglist[in_redirect + 1], O_RDONLY);
        if (in_fd < 0) { perror("open <"); return 1; }
        arglist[in_redirect] = NULL;
    }

    if (out_redirect != -1) {
        out_fd = open(arglist[out_redirect + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) { perror("open >"); return 1; }
        arglist[out_redirect] = NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
        if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }

        execvp(arglist[0], arglist);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }

    if (in_fd != -1) close(in_fd);
    if (out_fd != -1) close(out_fd);

    return 0;
}

