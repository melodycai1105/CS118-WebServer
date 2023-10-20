#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
using namespace std;

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = (char *)malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    char file_name[] = "index.html";
    
    string fn = "";
    int i = 5;
    // whitespace: space, tab, newlines
    // contains %
    // "GET /index.html HTTP/1.1\r\n"

    while(request[i]!='\r')
    {
        if(request[i]=='%')
        {
            if(request[i+1]=='2' && request[i+2]=='5')
            {
                fn += "%";
                i+=2;
            }
            else if (request[i+1]=='2' && request[i+2]=='0')
            {
                fn += " ";
                i+=2;
            }
            else{
                fn += request[i];
            }
        }
        else{
            fn += request[i];
        }
        i+=1;
    }

    size_t length = fn.size();
    size_t file_name_length = length-9;

    
    if(file_name_length)
    {
        string temp_fn = fn.substr(0,file_name_length); 
        size_t length = temp_fn.size();

        char* char_array = new char[file_name_length + 1]; 
        memcpy(char_array, temp_fn.c_str(), file_name_length+1);

        if(length>=3)
        {
            string extension = temp_fn.substr(length-3,length);
            if(extension==".ts")
            {
                proxy_remote_file(app, client_socket, char_array);
            }
        }
        else
        {
            serve_local_file(client_socket, char_array);
        }
    }
    else
    {
        serve_local_file(client_socket, file_name);
    }

    
    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    //serve_local_file(client_socket, file_name);

    //}
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    // char arr[strlen(path) + 1];  
    // strcpy(arr, path);

    bool is_extension = false;
    string file_extension = "";
    for (int i = strlen(path) - 1; i >= 0; i --){
        if (path[i] == '.'){
            int length = (strlen(path)-1) - i;
            char *extension = (char *)malloc(length);
            strcpy(extension, &path[i+1]);
            file_extension = extension;
            if (strcmp(extension, "html") == 0 || strcmp(extension, "txt") == 0 || strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0){
                is_extension = true;
            }
            if (strcmp(extension, "ts") == 0)
            {

            }
        }else{
            continue;
        }
    }

    string content_type = "";
    if (is_extension){
        if (file_extension == "txt"){
            content_type = "text/plain; charset=UTF-8";
        } else if (file_extension == "html"){
            content_type = "text/html; charset=UTF-8";
        } else if (file_extension == "jpg" || file_extension == "jpeg"){
            content_type =  "image/jpeg";
        }
    } else {
            content_type =  "application/octet-stream";
    }

    /*
    file.open(path, std::ios::binary | ios::in);
    if (file){
        status_code = "HTTP/1.0 200 OK";
        if (file.is_open()){
            while (file){
                getline(file, temp_line);
                response_body += temp_line + "\n";
            }
        }
    } else {
        status_code = "HTTP/1.0 404 NOT FOUND";
    }
*/
    string status_code = "HTTP/1.0 404 NOT FOUND";
    string content_length = "";
    FILE* fp;
    fp = fopen(path, "rb");

    if (!fp || fseek(fp, 0, SEEK_END) == -1)
    {
        //perror("failed to fseek %s\n");
        string response_str = status_code + "\r\n" + "Content-Type: " + content_type + "\r\n" + "Content-Length: " 
    + content_length + "\r\n\r\n";
        char response[response_str.size()];
        memcpy(response, response_str.c_str(), response_str.size());
        send(client_socket, response, strlen(response), 0);
        //return;
    } 
    else{
        status_code = "HTTP/1.0 200 OK";
        size_t off = ftell(fp);
        content_length = to_string(off);
        fclose(fp);
        fp = fopen(path, "rb");
        char buffer[off];
        fread(buffer,off,1,fp);

        string response_str = status_code + "\r\n" + "Content-Type: " + content_type + "\r\n" + "Content-Length: " 
    + content_length + "\r\n\r\n";
        char response[response_str.size()];
        memcpy(response, response_str.c_str(), response_str.size());
        send(client_socket, response, strlen(response), 0);
        
        send(client_socket, buffer, off, 0);
    }


    // int response_body_length= response_body.size();
    // response_body = response_body.substr(0,response_body_length -1);
  
    // if (file_extension == "jpg" || file_extension == "jpeg"){
    //     content_length = file_size(path(path));
    // } else {

    // }

    // multiple send
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
}