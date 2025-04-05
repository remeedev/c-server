#include <stdbool.h>

#ifndef hp_h
#define hp_h

typedef struct {
    char *path;
    char *operation;
}request_header;
bool is_get(request_header *req);
request_header* parse_header(char *content);
request_header* parse_from_file(char *file_name);
void free_header(request_header *req);

#endif
