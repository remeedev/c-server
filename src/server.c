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

char* load_html(char* file_name){
    FILE *html_file = fopen(file_name, "r");
    char *html_header = "HTTP/1.1 200 OK\r\nContent-Type=text/html; charset=utf-8\r\n\r\n";
    if (html_file == NULL){
        char *error = "Webpage was not found!";
        printf("[%s] file was not found!\n", file_name);
        return error;
    }
    long f_size;
    fseek(html_file, 0L, SEEK_END);
    f_size = ftell(html_file);
    rewind(html_file);

    char *file_text = (char *)malloc(f_size+1);
    if (file_text == NULL){
        fclose(html_file);
        printf("Error allocation memory reading [%s] file!\n", file_name);
        return "";
    }
    memset(file_text, 0, f_size+1);
    if(1!=fread(file_text, f_size, 1, html_file)){
        printf("Error reading the file contents!\n");
        free(file_text);
        fclose(html_file);
        return "";
    }
    char *return_text = (char *)malloc(f_size + strlen(html_header) + 1);
    strcpy(return_text, html_header);
    strcat(return_text, file_text);
    fclose(html_file);
    free(file_text);
    return return_text;
}

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

response* gen_response(char *recvd){
    response* out = (response *)malloc(sizeof(response));
    request_header* head = parse_header(recvd);
    if (strcmp(head->path, "/") == 0){
        char *out_html = load_html("./public/static/index.html");
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
        char *res = setup_vars(out_html, vars);
        free(for_out);
        free(for_info);
        out->response_size = strlen(res);
        out->out_msg = (char *)malloc(strlen(res)+1);
        strcpy(out->out_msg, res);
        free(res);
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
        if (strcmp(comp_ext, "ico")==0){
            char *image_path = "./public/images";
            char *full_path = (char *)malloc(strlen(image_path)+strlen(head->path)+1);
            strcpy(full_path, image_path);
            strcat(full_path, head->path);
            long image_size;
            char *image_info = load_image_info(full_path, &image_size);
            if (image_info == NULL){
                char *msg = (char *)malloc(strlen(full_path)+strlen("[] couldn't be found!")+1);
                sprintf(msg, "[%s] couldn't be found!", full_path);
                out->response_size = strlen(msg);
                out->out_msg = (char *)malloc(strlen(msg)+1);
                strcpy(out->out_msg, msg);
                free(full_path);
                free(comp_ext);
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
        if (strcmp(comp_ext, "css") == 0){
            char *file_path = "./public/style";
            char *full_path = (char *)malloc(strlen(file_path)+strlen(head->path)+1);
            strcpy(full_path, file_path);
            strcat(full_path, head->path);
            char *file_info = load_file(full_path);
            if (file_info == NULL){
                char *msg = (char *)malloc(strlen(full_path)+strlen("[] couldn't be found!")+1);
                sprintf(msg, "[%s] couldn't be found!", full_path);
                out->response_size = strlen(msg);
                out->out_msg = (char *)malloc(strlen(msg)+1);
                strcpy(out->out_msg, msg);
                free(full_path);
                free(comp_ext);
                free_header(head);
                return out;
            }
            char header[256];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type:text/css\r\n"
                     "Content-Length:%zu\r\n"
                     "Connection: close\r\n\r\n", strlen(file_info));
            size_t total_size = strlen(file_info)+strlen(header)+1;
            char *return_list = (char *)malloc(total_size);
            strcpy(return_list, header);
            strcat(return_list, file_info);
            out->response_size = total_size;
            out->out_msg = return_list;
            free(full_path);
            free(file_info);
            free(comp_ext);
            free_header(head);
            return out;
        }
        free(comp_ext);
    }
    out->response_size = 4;
    out->out_msg = (char *)malloc(5);
    strcpy(out->out_msg, "pene");
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
    while (running){
        receive_first(socket_fd);
        if (argc == 2){
            if (strcmp(argv[1], "once") == 0){
                running = false;
            }
        }
    }

    // Freeing all available memory
    freeaddrinfo(self_addr);
    close(socket_fd);
    return 0;
}
