#include "shell.h"

/* Circular buffer implementation for history */
static char* history[HISTORY_SIZE];
static int history_start = 0;  /* index of oldest entry */
static int history_count = 0;  /* number of entries currently stored */

/* Add a command to history (makes a copy) */
void add_history(const char* cmd) {
    if (!cmd) return;
    char *copy = strdup(cmd);
    if (!copy) return; /* allocation failed, silently return */

    if (history_count < HISTORY_SIZE) {
        int idx = (history_start + history_count) % HISTORY_SIZE;
        history[idx] = copy;
        history_count++;
    } else {
        /* overwrite oldest */
        free(history[history_start]);
        history[history_start] = copy;
        history_start = (history_start + 1) % HISTORY_SIZE;
    }
}

/* Print history lines numbered 1..history_count */
void print_history(void) {
    for (int i = 0; i < history_count; ++i) {
        int idx = (history_start + i) % HISTORY_SIZE;
        printf("%d  %s\n", i + 1, history[idx]);
    }
}

/* Return the nth command (1-based) or NULL if invalid */
const char* get_history(int n) {
    if (n < 1 || n > history_count) return NULL;
    int idx = (history_start + (n - 1)) % HISTORY_SIZE;
    return history[idx];
}

