#include <stdio.h>

#ifndef h_vars
#define h_vars

struct variable{
    char *name;
    char *value;

    struct variable *next;
};

struct variable *set_var(char *name, char *value, struct variable *start);
void free_all_vars(struct variable *first);
char *get_val(char *name, struct variable *start);
char *format_var(char *plaintext, int curr_pos, struct variable *first, char **var_out);
char *setup_vars(char *plaintext, struct variable *first);
struct variable **add_variable(struct variable **list, size_t new_length, struct variable* new_var);
char *load_list(char *template_text, struct variable **variables, size_t count);
char *load_file(char *file_name);
char *load_template_from_file(char *template_name, char *data_raw);
#endif
