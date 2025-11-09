/* src/vars.c -- Simple linked-list variable store for shell variables */

#include "shell.h"

typedef struct Var {
    char *name;
    char *value;
    struct Var *next;
} Var;

static Var *vars_head = NULL;

/* Helper: free one Var node */
static void free_var_node(Var *v) {
    if (!v) return;
    free(v->name);
    free(v->value);
    free(v);
}

/* set_variable: add or update */
void set_variable(const char *name, const char *value) {
    if (!name) return;
    /* name must be non-empty and start with a letter or underscore */
    if (name[0] == '\0') return;

    /* find existing */
    Var *cur = vars_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            free(cur->value);
            cur->value = strdup(value ? value : "");
            return;
        }
        cur = cur->next;
    }

    /* not found => create */
    Var *n = malloc(sizeof(Var));
    if (!n) return;
    n->name = strdup(name);
    n->value = strdup(value ? value : "");
    n->next = vars_head;
    vars_head = n;
}

/* get_variable: return value or NULL */
const char* get_variable(const char *name) {
    if (!name) return NULL;
    Var *cur = vars_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0)
            return cur->value;
        cur = cur->next;
    }
    return NULL;
}

/* print_variables: prints all stored variables */
void print_variables(void) {
    Var *cur = vars_head;
    while (cur) {
        printf("%s=%s\n", cur->name, cur->value);
        cur = cur->next;
    }
}

/* free_variables: free list at shell exit */
void free_variables(void) {
    Var *cur = vars_head;
    while (cur) {
        Var *next = cur->next;
        free_var_node(cur);
        cur = next;
    }
    vars_head = NULL;
}

/* expand_arglist: replace any token that starts with '$' with var value */
void expand_arglist(char **arglist) {
    if (!arglist) return;
    for (int i = 0; arglist[i] != NULL; ++i) {
        char *tok = arglist[i];
        if (!tok || tok[0] != '$') continue;

        const char *varname = tok + 1; /* skip $ */
        if (*varname == '\0') {
            /* token was just "$" -> replace with empty string */
            free(arglist[i]);
            arglist[i] = strdup("");
            continue;
        }

        const char *val = get_variable(varname);
        if (val) {
            free(arglist[i]);
            arglist[i] = strdup(val);
        } else {
            free(arglist[i]);
            arglist[i] = strdup(""); /* undefined -> empty string */
        }
    }
}

