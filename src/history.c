/* src/history.c */
#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define HISTORY_SIZE 20

static char* hist_buf[HISTORY_SIZE];
static int hist_start = 0;
static int hist_count = 0;

/* Add command to custom history (20-entry circular buffer) */
void add_to_history(const char* cmd) {
    if (!cmd || *cmd == '\0') return;
    char *copy = strdup(cmd);
    if (!copy) return;

    if (hist_count < HISTORY_SIZE) {
        int idx = (hist_start + hist_count) % HISTORY_SIZE;
        hist_buf[idx] = copy;
        hist_count++;
    } else {
        free(hist_buf[hist_start]);
        hist_buf[hist_start] = copy;
        hist_start = (hist_start + 1) % HISTORY_SIZE;
    }
}

/* Show custom history numbered 1..n */
void show_history(void) {
    for (int i = 0; i < hist_count; ++i) {
        int idx = (hist_start + i) % HISTORY_SIZE;
        printf("%d  %s\n", i + 1, hist_buf[idx]);
    }
}

/* Return nth command (1-based) or NULL */
char* get_history_command(int n) {
    if (n < 1 || n > hist_count) return NULL;
    int idx = (hist_start + (n - 1)) % HISTORY_SIZE;
    return hist_buf[idx];
}

/* Free buffer at exit */
void cleanup_history(void) {
    for (int i = 0; i < hist_count; ++i) {
        int idx = (hist_start + i) % HISTORY_SIZE;
        free(hist_buf[idx]);
        hist_buf[idx] = NULL;
    }
    hist_count = 0;
    hist_start = 0;
}

