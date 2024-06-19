#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PATH_SIZE 256

void send_404(int client_socket) {
    const char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>404 Not Found</title></head>"
        "<body><h1>404 Not Found</h1></body>"
        "</html>";
    send(client_socket, response, strlen(response), 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    // GET
    if (strncmp(buffer, "GET", 3) == 0) {
        char method[5], path[MAX_PATH_SIZE], protocol[10];
        sscanf(buffer, "%s %s %s", method, path, protocol);

        // Delete /
        char *file_name = path + 1;
        char *dot = strrchr(file_name, '.');
        if (dot) {
            *dot = '\0';
        }

        if (strcmp(file_name, "") == 0) {
            strcpy(file_name, "index");
        }

        char file_path[MAX_PATH_SIZE];
        snprintf(file_path, sizeof(file_path), "%s.html", file_name);

        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
            send_404(client_socket);
        } else {
            struct stat st;
            stat(file_path, &st);
            int file_size = st.st_size;
            char *file_content = malloc(file_size);

            fread(file_content, 1, file_size, file);
            fclose(file);

            char header[BUFFER_SIZE];
            sprintf(header, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n", file_size);

            send(client_socket, header, strlen(header), 0);
            send(client_socket, file_content, file_size, 0);
            free(file_content);
        }
    } else {
        send_404(client_socket);
    }

    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) < 0) { // SOMAXCONN = 128
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}
