#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *path;
    char *operation;
} request_header;

bool is_get(request_header *req){
    return strcmp(req->operation, "GET") == 0;
}

size_t *increase_size(size_t *list, int new_length){
    size_t *temp = realloc(list, new_length*sizeof(size_t));
    if (temp == NULL){
        printf("Error in memory allocation!\n");
        free(list);
        exit(1);
    }
    list = temp;
    return list;
}

char *substr(char *str, int pos, int size){
    char *out = (char *)malloc(size*(sizeof(char)+1));
    strcpy(out, "");
    int tempted = 0;
    while (tempted < size){
        out[tempted] = str[pos+tempted];
        tempted++;
    }
    out[tempted] = '\0';
    return out;
}

char **split(char *str, char *split, size_t *length){
    int count = 0;
    int current_count = 0;
    size_t list_size = 1;
    size_t *item_lists = (size_t *)calloc(1, sizeof(size_t));

    while (str[count] != '\0'){
        char *_str = substr(str, count, strlen(split));
        if (strcmp(_str, split) == 0){
            item_lists[list_size-1] = current_count;
            list_size++;
            item_lists = increase_size(item_lists, list_size);
            current_count = 0;
            count = count + strlen(split);
        }
        free(_str);
        count++;
        current_count++;
    }
    item_lists[list_size-1] = current_count;

    char **out = (char **)malloc(sizeof(char *)*list_size);
    out[0] = "";

    count = 0;
    for (int i = 0; i < list_size; i++){
        char *elem = substr(str, count, item_lists[i]);
        count = count + item_lists[i] + strlen(split);
        out[i] = elem;
    }
    free(item_lists);
    *length = list_size;
    return out;
}

void free_split(char** split_out, size_t size){
    if (split_out == NULL){
        return;
    }
    for (int i = 0; i < size; i++){
        free(split_out[i]);
    }
    free(split_out);
}

request_header* parse_header(char *content){
    size_t out_size = 0;
    char** header_split = split(content, "\r\n", &out_size);
    request_header *req = (request_header *)malloc(sizeof(request_header));
    for (int i = 0; i < out_size; i++){
        if (i < 1){
            size_t indexes = 0;
            char** space_separated = split(header_split[i], " ", &indexes);
            size_t oper_len = sizeof(char)*(strlen(space_separated[0])+1);
            size_t path_len = sizeof(char)*(strlen(space_separated[1])+1);
            req->operation = (char *)malloc(oper_len);
            req->path = (char *)malloc(path_len);
            strncpy(req->operation, space_separated[0], oper_len);
            strncpy(req->path, space_separated[1], path_len);
            free_split(space_separated, indexes);
        }
    }
    free_split(header_split, out_size);
    return req;
}

request_header* parse_from_file(char *file_name){
    FILE *header = fopen(file_name, "r");
    if (header == NULL){
        printf("[%s] is non existent!\n", file_name);
        return NULL;
    }
    char content[1024] = "";
    char _read[100];
    while(fgets(_read, 100, header)){
        strcat(content, _read);
    }
    request_header* req = parse_header(content);
    fclose(header);
    return req;
}

void free_header(request_header *req){
    free(req->operation);
    free(req->path);
    free(req);
}
