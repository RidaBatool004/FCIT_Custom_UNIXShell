#include "shell.h"

Job jobs[MAX_JOBS];
int job_count = 0;

void add_job(pid_t pid, const char *cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmd, cmd, 255);
        jobs[job_count].cmd[255] = '\0';
        job_count++;
    }
}

void reap_terminated_jobs() {
    int status;
    pid_t pid;
    for (int i = 0; i < job_count; ) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG);
        if (pid > 0) {
            printf("[+] Background job finished: %s (PID: %d)\n", jobs[i].cmd, jobs[i].pid);
            // remove finished job
            for (int j = i; j < job_count - 1; j++)
                jobs[j] = jobs[j + 1];
            job_count--;
        } else {
            i++;
        }
    }
}

void show_jobs() {
    printf("\nActive background jobs:\n");
    if (job_count == 0) {
        printf("  (none)\n");
        return;
    }
    for (int i = 0; i < job_count; i++) {
        printf("  [%d] PID: %d  CMD: %s\n", i + 1, jobs[i].pid, jobs[i].cmd);
    }
}

int execute(char *arglist[]) {
    int in_redirect = -1, out_redirect = -1, pipe_pos = -1, background = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) in_redirect = i;
        else if (strcmp(arglist[i], ">") == 0) out_redirect = i;
        else if (strcmp(arglist[i], "|") == 0) pipe_pos = i;
        else if (strcmp(arglist[i], "&") == 0) {
            background = 1;
            arglist[i] = NULL;
        }
    }

    // PIPE handling
    if (pipe_pos != -1) {
        arglist[pipe_pos] = NULL;
        char **left_cmd = arglist;
        char **right_cmd = &arglist[pipe_pos + 1];

        int fds[2];
        if (pipe(fds) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t left_pid = fork();
        if (left_pid == 0) {
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
            execvp(left_cmd[0], left_cmd);
            perror("execvp left");
            exit(1);
        }

        pid_t right_pid = fork();
        if (right_pid == 0) {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            execvp(right_cmd[0], right_cmd);
            perror("execvp right");
            exit(1);
        }

        close(fds[0]);
        close(fds[1]);

        if (!background) {
            waitpid(left_pid, NULL, 0);
            waitpid(right_pid, NULL, 0);
        } else {
            printf("[+] Running in background (pipe): PID %d, %d\n", left_pid, right_pid);
            add_job(left_pid, left_cmd[0]);
            add_job(right_pid, right_cmd[0]);
        }
        return 0;
    }

    // Redirection handling
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

    // Execution
    pid_t pid = fork();
    if (pid == 0) {
        if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
        if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
        execvp(arglist[0], arglist);
        perror("execvp");
        exit(127); // return 127 if command not found
    } 
    else if (pid > 0) {
        int status = 0;
        if (background) {
            printf("[+] Running in background: PID %d\n", pid);
            add_job(pid, arglist[0]);
            return 0;
        } else {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
        }
    } 
    else {
        perror("fork");
        return 1;
    }

    if (in_fd != -1) close(in_fd);
    if (out_fd != -1) close(out_fd);

    return 0;
}
int execute_with_status(char *arglist[], int *exit_status) {
    if (!arglist || !arglist[0]) {
        if (exit_status) *exit_status = 1;
        return 1;
    }

    int in_redirect = -1, out_redirect = -1, pipe_pos = -1;

    // detect <, >, | 
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) in_redirect = i;
        else if (strcmp(arglist[i], ">") == 0) out_redirect = i;
        else if (strcmp(arglist[i], "|") == 0) pipe_pos = i;
    }

    // Handle simple pipe (no background for conditions)
    if (pipe_pos != -1) {
        arglist[pipe_pos] = NULL;
        char **left_cmd = arglist;
        char **right_cmd = &arglist[pipe_pos + 1];

        int fds[2];
        if (pipe(fds) == -1) {
            if (exit_status) *exit_status = 1;
            return 1;
        }

        pid_t left_pid = fork();
        if (left_pid == 0) {
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
            execvp(left_cmd[0], left_cmd);
            _exit(127);
        }

        pid_t right_pid = fork();
        if (right_pid == 0) {
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
            execvp(right_cmd[0], right_cmd);
            _exit(127);
        }

        close(fds[0]);
        close(fds[1]);
        int status = 0;
        waitpid(left_pid, &status, 0);
        waitpid(right_pid, &status, 0);
        if (exit_status)
            *exit_status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
        return *exit_status;
    }

    // handle redirection
    int in_fd = -1, out_fd = -1;
    if (in_redirect != -1) {
        in_fd = open(arglist[in_redirect + 1], O_RDONLY);
        if (in_fd < 0) { if (exit_status) *exit_status = 1; return 1; }
        arglist[in_redirect] = NULL;
    }
    if (out_redirect != -1) {
        out_fd = open(arglist[out_redirect + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) { if (exit_status) *exit_status = 1; return 1; }
        arglist[out_redirect] = NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
        if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
        execvp(arglist[0], arglist);
        _exit(127);
    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (exit_status)
            *exit_status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    } else {
        if (exit_status) *exit_status = 1;
        return 1;
    }

    if (in_fd != -1) close(in_fd);
    if (out_fd != -1) close(out_fd);

    return (exit_status ? *exit_status : 0);
}

