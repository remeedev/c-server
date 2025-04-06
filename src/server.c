#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include "hp.h"
#include "h_vars.h"

#define PORT 3000
#define BACKLOG 10

struct addrinfo *get_def_addr(){
    struct addrinfo *self_addr;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    char *port = (char*)malloc(7);
    sprintf(port, "%d", PORT);
    if ((status = getaddrinfo(NULL, port, &hints, &self_addr)) == -1){
        printf("Error getting address info!\n");
        exit(1);
    }
    free(port);
    return self_addr;
}

int get_socket(struct addrinfo *address){
    int socket_fd;
    if ((socket_fd =socket(address->ai_family, address->ai_socktype, 0)) == -1){
        printf("Error generating the socket file descriptor!\n");
        exit(1);
    }
    return socket_fd;
}

void assure_bind(int socket_fd, struct addrinfo *address){
    if (bind(socket_fd, address->ai_addr, address->ai_addrlen) == -1){
        printf("Error binding socket!\n");
        exit(1);
    }
}

void start_listening(int socket_fd){
    if (listen(socket_fd, BACKLOG) == -1){
        printf("Error opening socket to listen!\n");
        exit(1);
    }
}

typedef struct{
    char *out_msg;
    size_t response_size;
} response;

char* load_image_info(char *image_path, long *size_out){
    FILE *file = fopen(image_path, "rb");
    if (file == NULL){
        printf("[%s] was not found!\n", image_path);
        return NULL;
    }
    long file_size;
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    if (file_size <= 0){
        printf("empty or invalid image!\n");
        fclose(file);
        return NULL;
    }
    char *data = (char *)malloc(file_size);
    if (data ==NULL){
        printf("Error allocating memory for image!\n");
        fclose(file);
        return NULL;
    }
    size_t read_bytes = fread(data, 1, file_size, file);
    fclose(file);
    if (read_bytes != file_size){
        printf("read [%zu] of [%zu] bytes\n", read_bytes, file_size);
        return NULL;
    }
    *size_out = read_bytes;
    return data; 
}

char *create_image_header(char *extension_type, char *image_information, long size, size_t *total_size){
    if (strcmp(extension_type, "ico")==0){
        char header[256];
        snprintf(header, sizeof(header), 
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: image/x-icon\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: close\r\n"
                 "\r\n", size);
        size_t header_len = strlen(header);
        size_t net_size = header_len + size;
        char * response = (char *)malloc(net_size);
        if (response == NULL){
            printf("Error allocating space for response!\n");
            return "";
        }
        memcpy(response, header, header_len);
        memcpy(response+header_len, image_information, size);
        *total_size = net_size;
        return response;
    }
    return "";
}

char *plaintext_response(char *text){
    char header_buff[512];
    snprintf(header_buff, sizeof(header_buff), 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type:text/plain\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n", strlen(text));
    char *out = (char *)malloc(strlen(text)+strlen(header_buff)+1);
    strcpy(out, header_buff);
    strcat(out, text);
    return out;
}

char *load_content(char *content, char *extension){
    char *extensions[] = {"css", "txt", "js", "html", NULL};
    char *MIME[] = {"text/css", "text/plain", "text/javascript", "text/html", NULL};
    char *mime_out = NULL;
    int pos = 0;
    while (extensions[pos] != NULL){
        if (strcmp(extension, extensions[pos]) == 0){
            mime_out = MIME[pos];
        }
        pos++;
    }
    if (mime_out == NULL){
        return NULL;
    }
    char header_buff[512];
    snprintf(header_buff, sizeof(header_buff),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type:%s\r\n"
             "Content-Length:%zu\r\n"
             "Connection: close\r\n"
             "\r\n", mime_out, strlen(content));
    size_t total_size = strlen(content)+strlen(header_buff)+1;
    char *out = (char *)malloc(total_size);
    if (out == NULL){
        return NULL;
    }
    memset(out, 0, total_size);
    strcpy(out, header_buff);
    strcat(out, content);
    return out;
}

char *load_text_file(char *file_name, char *extension){
    char  *check = load_content("", extension);
    if (check == NULL){
        return NULL;
    }
    free(check);
    FILE *file = fopen(file_name, "r");
    if (file == NULL){
        char buff[100];
        snprintf(buff, sizeof(buff), "[%s] was not found or was wrongly opened!", file_name);
        return plaintext_response(buff);
    }
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *in_text = (char *)malloc(file_size+1);
    if (in_text == NULL){
        printf("Error allocating memory for text!");
        fclose(file);
        return plaintext_response("Error allocating memory for text!");
    }
    memset(in_text, 0, file_size);
    if(1!=fread(in_text, file_size, 1, file)){
        printf("Error reading the file contents!\n");
        free(in_text);
        fclose(file);
        return plaintext_response("Error reading file contents!");
    }
    in_text[file_size] = '\0';
    char *out = load_content(in_text, extension);
    free(in_text);
    fclose(file);
    return out;
}

response* gen_response(char *recvd){
    response* out = (response *)malloc(sizeof(response));
    request_header* head = parse_header(recvd);
    //printf("[%s] requested!\n", head->path);
    if (strcmp(head->path, "/") == 0){
        char *out_html = load_file("./public/static/index.html");
        struct variable *vars;
        if (strlen(head->path) == 1){
            vars = set_var("user_defined", "new user", NULL);
        }else{
            vars = set_var("user_defined", (head->path+1), NULL);
        }
        set_var("website_defined", "testing", vars);
        char *for_info = load_file("./public/static/test.txt");
        char *for_out = load_template_from_file("./public/static/template_list.html", for_info);
        set_var("for_test", for_out, vars);
        char *_res = setup_vars(out_html, vars);
        char *res = load_content(_res, "html");
        free(_res);
        out->response_size = strlen(res);
        out->out_msg = res;
        free(for_out);
        free(for_info);
        free_all_vars(vars);
        free(out_html);
        free_header(head);
        return out;
    }
    bool extension = false;
    bool path_f = false;
    int curr_pos = 0;
    int period_pos = -1;
    size_t n_size = 0;
    size_t x_size = 0;
    while (head->path[curr_pos] != '\0' && !path_f){
        if (head->path[curr_pos] == '.'){
            extension = true;
            period_pos = curr_pos;
        }
        if (head->path[curr_pos] == '/' && curr_pos != 0){
            path_f = true;
        }else{
            if (extension && head->path[curr_pos] != '.'){
                x_size++;
            }else{
                if (extension == false && head->path[curr_pos] != '/'){
                    n_size++;
                }
            }
        }
        curr_pos++;
    }
    if (extension){
        char *comp_ext = (char *)malloc(x_size+1);
        int start = period_pos+1;
        int _i = 0;
        while(head->path[start] != '/' && head->path[start] != '\0'){
            comp_ext[_i] = head->path[start];
            _i++;
            start++;
        }
        comp_ext[_i] = '\0';
        char *full_path;
        char *path_alloc[] = {"ico", "css", "js", "txt", NULL};
        char *paths[] = {"./public/images", "./public/style", "./public/scripts", "./public/static", NULL};
        char *temporary_path = NULL;
        bool found = false;
        int pos = 0;
        while (path_alloc[pos] != NULL && !found){
            if (strcmp(comp_ext, path_alloc[pos]) == 0){
                temporary_path = (char *)malloc(strlen(paths[pos]) + strlen(head->path) +1);
                if (temporary_path == NULL){
                    printf("Not able to allocate memory for path!\n");
                    out->out_msg = plaintext_response("Not able to allocate memory for path!");
                    out->response_size = strlen(out->out_msg);
                    free_header(head);
                    free(comp_ext);
                    return out;
                }
                strcpy(temporary_path, paths[pos]);
                strcat(temporary_path, head->path);
                found = true;
            }
            pos++;
        }
        if (temporary_path == NULL){
            out->out_msg = plaintext_response("File path not supported!");
            out->response_size = strlen(out->out_msg);
            free_header(head);
            free(comp_ext);
            return out;
        }
        char *response_text = load_text_file(temporary_path, comp_ext);
        if (response_text != NULL){
            out->out_msg = response_text;
            out->response_size = strlen(out->out_msg);
            free(comp_ext);
            free_header(head);
            free(temporary_path);
            return out;
        }
        if (strcmp(comp_ext, "ico")==0){
            char *full_path = temporary_path;
            long image_size;
            char *image_info = load_image_info(full_path, &image_size);
            if (image_info == NULL){
                char *msg = (char *)malloc(strlen(full_path)+strlen("[] couldn't be found!")+1);
                sprintf(msg, "[%s] couldn't be found!", full_path);
                out->out_msg = plaintext_response(msg);
                out->response_size = strlen(out->out_msg);
                free(full_path);
                free(comp_ext);
                free(msg);
                free_header(head);
                return out;
            }
            size_t out_size;
            char *out_msg = create_image_header(comp_ext, image_info, image_size, &out_size);
            out->response_size = out_size;
            out->out_msg = out_msg;
            free(image_info);
            free(full_path);
            free(comp_ext);
            free_header(head);
            return out;
        }
        
        free(comp_ext);
    }
    out->out_msg = plaintext_response("Path not able to be redirected!");
    out->response_size = strlen(out->out_msg);
    free_header(head);
    return out;
}

void free_response(response *res){
    free(res->out_msg);
    free(res);
}

void receive_first(int socket_fd){
    struct sockaddr_storage peer_addr;
    socklen_t addr_size = sizeof peer_addr;
    int new_fd = accept(socket_fd, (struct sockaddr *)&peer_addr, &addr_size);
    if (new_fd == -1){
        printf("Error accepting request!\n");
        exit(1);
    }
    size_t buff_len = 1024;
    char *recvd = (char *)malloc(buff_len);

    int recv_size;

    if ((recv_size=recv(new_fd, recvd, buff_len, 0)) == -1){
        printf("Error receiving message!\n");
        exit(1);
    }
    if (recv_size == 0){
        printf("Peer closed connection!\n");
        free(recvd);
        return;
    }
    response *res = gen_response(recvd);

    if (send(new_fd, res->out_msg, res->response_size, 0) == -1){
        printf("Error sending response!\n");
        exit(1);
    }
    free_response(res);

    free(recvd);
    close(new_fd);
}

void setup_options(int socket_fd){
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1){
        printf("Error setting socket options!\n");
        exit(1);
    }
}

int main(int argc, char *argv[]){
    printf("Starting server!\n");
    // Basic setup for server
    int socket_fd;
    struct addrinfo *self_addr = get_def_addr();
    socket_fd = get_socket(self_addr);
    setup_options(socket_fd);
    assure_bind(socket_fd, self_addr);
    start_listening(socket_fd);
    printf("Listening in on port [%d]\n", PORT);

    // receiving information
    bool running = true;
    int count = 0;
    int max_count;
    while (running){
        receive_first(socket_fd);
        if (argc == 2){
            if (strcmp(argv[1], "once") == 0){
                running = false;
            }else{
                count++;
                printf("Request %d fulfilled!\n", count);
                max_count = atoi(argv[1]);
                if (count >= max_count){
                    running = false;
                }
            }
        }
    }

    // Freeing all available memory
    freeaddrinfo(self_addr);
    close(socket_fd);
    return 0;
}
