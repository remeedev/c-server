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

response* gen_response(char *recvd){
    response* out = (response *)malloc(sizeof(response));
    request_header* head = parse_header(recvd);
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
    free_all_vars(vars);
    out->response_size = strlen(res);
    out->out_msg = (char *)malloc(strlen(res)+1);
    strcpy(out->out_msg, res);
    free(res);
    free_header(head);
    free(out_html);
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
