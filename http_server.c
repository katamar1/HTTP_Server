#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define BUFFER_SIZE 1024

/* this handles a single client's request, takes in an int (the client's socket) and does the correct thing depending on the client. no return value */
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char *request = NULL;
    size_t request_size = 0;
    ssize_t bytes_read;

    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        request_size += bytes_read;
        request = realloc(request, request_size + 1);
        if (!request) {
            perror("Memory allocation failed");
            close(client_socket);
            return;
        }

        strcat(request, buffer);

        if (strstr(request, "\r\n\r\n") != NULL) {
            char method[8], path[1024];
            sscanf(request, "%s %s", method, path);

            if (strcmp(method, "GET") != 0) {
                const char *response_405=
                    "HTTP/1.1 405 This Method is Forbidden\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
                send(client_socket, response_405, strlen(response_405), 0);
                break;
            }

            if (strcmp(path, "/") == 0) {
                strcpy(path, "/index.html");
            }

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "./public%s", path);
            printf("Full path to open: %s\n", full_path);

            FILE *fp = fopen(full_path, "r");
            if (!fp) {
                FILE *fp_404 = fopen("./public/404.html", "r");
                if (fp_404) {
                    fseek(fp_404, 0, SEEK_END);
                    long error_file_size = ftell(fp_404);
                    rewind(fp_404);

                    char *error_content = malloc(error_file_size);
                    if (error_content && fread(error_content, 1, error_file_size, fp_404) == error_file_size) {
                        char error_header[256];
                        snprintf(error_header, sizeof(error_header),
                            "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %ld\r\n"
                            "Connection: close\r\n\r\n",
                            error_file_size);

                        send(client_socket, error_header, strlen(error_header), 0);
                        send(client_socket, error_content, error_file_size, 0);
                    } else {
                        const char *fallback_404 =
                            "HTTP/1.1 404 Not Found\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: 83\r\n"
                            "Connection: close\r\n\r\n"
                            "<html><body><h1>404 Not Found</h1><p>Could not find the requested file.</p></body></html>";
                        send(client_socket, fallback_404, strlen(fallback_404), 0);
                    }

                    if (error_content) free(error_content);
                    fclose(fp_404);
                } else {
                    const char *fallback_404 =
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 83\r\n"
                        "Connection: close\r\n\r\n"
                        "<html><body><h1>404 Not Found</h1><p>Could not find the requested file.</p></body></html>";
                    send(client_socket, fallback_404, strlen(fallback_404), 0);
                }

                break;
            }

            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            rewind(fp);

            char *file_content = malloc(file_size);
            if (!file_content) {
                const char *response_500 =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
                send(client_socket, response_500, strlen(response_500), 0);
                fclose(fp);
                break;
            }

            size_t bytes_read_file = fread(file_content, 1, file_size, fp);
            fclose(fp);

            if (bytes_read_file != file_size) {
                const char *response_500 =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
                send(client_socket, response_500, strlen(response_500), 0);
                free(file_content);
                break;
            }

            char header[256];
            snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n\r\n",
                file_size);

            send(client_socket, header, strlen(header), 0);
            send(client_socket, file_content, file_size, 0);
            free(file_content);
        }
    }

    if (bytes_read < 0) {
        perror("recv failed");
    } else if (bytes_read == 0) {
        printf("Client disconnected unexpectedly.\n");
    }

    free(request);
    close(client_socket);
}

/* this creates the listening socket, binds to localhost:8080, listens for connections, and uses fork to create child processes for handling specific clients */
int main() {
    int socket_connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_connection == -1) {
        perror("socket connection failed!");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(socket_connection, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8080);
        addr.sin_addr.s_addr = INADDR_ANY;
        memset(&(addr.sin_zero), 0, 8);
    }

    int binded = bind(socket_connection, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (binded == -1) {
        perror("bind failed!");
        exit(EXIT_FAILURE);
    }

    int listen_connection = listen(socket_connection, 5);
    if (listen_connection == -1) {
        perror("listen failed!");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 8080...\n");

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int accept_connection = accept(socket_connection, (struct sockaddr *)&client_addr, &client_addr_len);
        if (accept_connection == -1) {
            perror("accept failed!");
            continue;
        }

        printf("Client connected.\n");

        pid_t pid = fork();
        if (pid == 0) {
            close(socket_connection);
            handle_client(accept_connection);
            exit(0);
        } else if (pid > 0) {
            close(accept_connection);
        } else {
            perror("fork failed!");
        }
    }
}







    



