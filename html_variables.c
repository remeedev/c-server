#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct variable{
    char *name;
    char *value;

    struct variable *next;
};

struct variable *set_var(char* name, char* value, struct variable *start){
    struct variable *new_var= (struct variable *)malloc(sizeof(struct variable));
    new_var->name = (char *)malloc(strlen(name)+1);
    new_var->value = (char *)malloc(strlen(value)+1);
    strcpy(new_var->name, name);
    strcpy(new_var->value, value);
    new_var->next = NULL;
    if (start == NULL){
        return new_var;
    }
    struct variable *tmp = start;
    while (tmp->next != NULL){
        tmp = tmp->next;
    }
    tmp->next = new_var;
    return start;
}

void free_all_vars(struct variable *first){
    if (first == NULL){
        return;
    }
    struct variable *tmp = first;
    while (tmp != NULL){
        struct variable* curr = tmp;
        tmp = tmp->next;
        free(curr->name);
        free(curr->value);
        free(curr);
    }
}

char *get_val(char *name, struct variable *start){
    struct variable *tmp = start;
    while (tmp != NULL){
        if (strcmp(tmp->name, name) == 0){
            char *out = (char *)malloc(strlen(tmp->value)+1);
            strcpy(out, tmp->value);
            return out;
        }
        tmp = tmp->next;
    }
    char *literal = (char *)malloc(strlen(name)+3);
    sprintf(literal, "{%s}", name);
    return literal;
}

char *format_var(char *plaintext, int curr_pos, struct variable *first, char **var_out){
    int _counter = 0;
    while (plaintext[curr_pos+_counter] != '}' && plaintext[curr_pos+_counter] != '\0'){
        _counter++;
    }
    char *var_name = (char *)malloc(_counter+1);
    int __counter = 0;
    while (__counter < _counter-1){
        var_name[__counter] = plaintext[curr_pos+__counter+1];
        __counter++;
    }
    var_name[__counter] = '\0';
    char *actual_value = get_val(var_name, first);
    if (var_out != NULL){
        *var_out = var_name;
    }else{
        free(var_name);
    }
    return actual_value;
}

struct variable **add_variable(struct variable **list, size_t new_length, struct variable* new_var){
    if (list == NULL){
        struct variable **list = (struct variable **)malloc(sizeof(struct variable*));
        list[0] = new_var;
        return list;
    }
    struct variable **new_list = realloc(list, new_length*sizeof(struct variable*));
    if (new_list == NULL){
        printf("Error increasing size of list\n");
        free(list);
        exit(1);
    }
    new_list[new_length-1] = new_var;
    return new_list;
}

void free_list(struct variable **list, size_t length){
    int curr_pos = 0;
    while (curr_pos < length){
        free_all_vars(list[curr_pos]);
        curr_pos++;
    }
    free(list);
}

char *setup_vars(char *plaintext, struct variable *first){
    if (first == NULL){
        return plaintext;
    }
    size_t curr_pos = 0;
    size_t end_len = 0;
    while (plaintext[curr_pos] != '\0'){
        if (plaintext[curr_pos] == '{'){
            char *var_name;
            char *actual_value = format_var(plaintext, curr_pos, first, &var_name);
            curr_pos = curr_pos+strlen(var_name)+1;
            end_len = end_len+strlen(actual_value)-1;
            free(var_name);
            free(actual_value);
        }
        curr_pos++;
        end_len++;
    }
    end_len++;
    char* out = (char *)malloc(end_len);
    memset(out, 0, end_len);
    size_t actual_len = curr_pos;
    size_t f_pos = 0;
    curr_pos = 0;
    while(curr_pos < actual_len){
        if (plaintext[curr_pos] == '{'){
            char *var_name;
            char *actual_value = format_var(plaintext, curr_pos, first, &var_name);
            curr_pos = curr_pos+strlen(var_name)+1;
            f_pos = f_pos+strlen(actual_value);
            strcat(out, actual_value);
            free(var_name);
            free(actual_value);
        }else{
            out[f_pos] = plaintext[curr_pos];
            f_pos++;
        }
        curr_pos++;
    }
    out[end_len-1] = '\0';
    return out;
}

char *load_list(char *template_text, struct variable **variables, size_t count){
    if (count == 0){
        return "";
    }
    int current = 0;
    char *out = setup_vars(template_text, variables[current]);
    current++;
    while (current < count){
        char *extra = setup_vars(template_text, variables[current]);
        char *temp = realloc(out, strlen(out)+strlen(extra)+2);
        if (temp == NULL){
            printf("Adding extra template memory allocation failed!\n");
            exit(1);
        }
        out = temp;
        strcat(out, "\n");
        strcat(out, extra);
        free(extra);
        current++;
    }
    char *temp = realloc(out, strlen(out)+2);
    if (temp == NULL){
        printf("Memory allocation error padding response!\n");
        exit(1);
    }
    out = temp;
    strcat(out, "\n");
    return out; 
}

char *load_file(char *file_name){
    FILE *file= fopen(file_name, "r");
    if (file == NULL){
        printf("[%s] does not exist!\n", file_name);
        return "";
    }

    long file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    char *file_text = (char *)malloc(file_size+1);
    if (file_text == NULL){
        fclose(file);
        printf("Error allocating memory for template_text!\n");
        return "";
    }
    memset(file_text, 0, file_size+1);
    if (1!=fread(file_text, file_size, 1, file)){
        printf("Error reading template text!\n");
        free(file_text);
        fclose(file);
        return "";
    }
    fclose(file);
    return file_text;
}

char *load_template_from_file(char *template_name, char *data_raw){
    size_t curr_pos = 0;
    struct variable *curr_var = NULL;
    struct variable **list = NULL;
    size_t list_size = 0;
    while (data_raw[curr_pos] != '\0'){
        size_t name_length = 0;
        size_t value_length = 0;
        size_t prev_pos = curr_pos;
        char *var_name;
        char *var_value;
        while (data_raw[curr_pos] != ':'){
            name_length++;
            curr_pos++;
        }
        curr_pos++;
        while (data_raw[curr_pos] != '\n'){
            value_length++;
            curr_pos++;
        }
        var_name = (char *)malloc(name_length+1);
        if (var_name == NULL){
            printf("Error allocating memory for var_name!\n");
            return "";
        }
        var_value = (char *)malloc(value_length+1);
        if (var_value == NULL){
            printf("Error allocating memory for var_value!\n");
            free(var_name);
            return "";
        }
        memset(var_name, 0, name_length+1);
        memset(var_value, 0, value_length+1);
        curr_pos = prev_pos;
        size_t _i = 0;
        while (data_raw[curr_pos] != ':'){
            var_name[_i] = data_raw[curr_pos];
            _i++;
            curr_pos++;
        }
        var_name[_i] = '\0';
        curr_pos++;
        _i = 0;
        while (data_raw[curr_pos] != '\n'){
            var_value[_i] = data_raw[curr_pos];
            _i++;
            curr_pos++;
        }
        var_value[_i] = '\0';
        if (curr_var == NULL){
            curr_var = set_var(var_name, var_value, curr_var);
        }else{
            set_var(var_name, var_value, curr_var);
        }
        free(var_name);
        free(var_value);
        curr_pos++;
        if (data_raw[curr_pos] == '\n'){
            list_size++;
            list = add_variable(list, list_size, curr_var);
            curr_var = NULL;
            curr_pos++;
        }
    }
    char *template_text = load_file(template_name);
    char *out = load_list(template_text, list, list_size);
    free(template_text);
    free_list(list, list_size);
    return out;
}
