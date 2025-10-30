// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#include "./constants.h"

void handle_client(int client_sock);

int main()
{
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Unlink any old socket file
    unlink(SOCKET_PATH);

    // 1. Create a UNIX domain socket
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Bind the socket to a file path
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_sock, 5) == -1)
    {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %s\n", SOCKET_PATH);

    // Handle zombie processes
    signal(SIGCHLD, SIG_IGN);

    // 4. Accept connections in a loop
    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1)
        {
            perror("accept");
            continue; // Continue to the next connection attempt
        }

        printf("Server: Accepted a new connection.\n");

        // 5. Fork a new process to handle the client
        if (fork() == 0)
        { // This is the child process
            close(server_sock); // Child doesn't need the listener socket
            handle_client(client_sock);
            exit(EXIT_SUCCESS);
        }
        else
        { 
            // This is the parent process
            close(client_sock); // Parent doesn't need the client socket
        }
    }

    // Cleanup (this part is unreachable in this simple example)
    close(server_sock);
    unlink(SOCKET_PATH);

    return 0;
}

// This function handles communication with a single client
void handle_client(int client_sock)
{
    char buffer[BUFFER_SIZE];
    int n;

    while ((n = read(client_sock, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[n] = '\0';
        printf("Server received: %s\n", buffer);

        // Echo the message back to the client
        if (write(client_sock, buffer, n) == -1)
        {
            perror("write");
            break;
        }
    }

    if (n == 0)
    {
        printf("Client disconnected.\n");
    } else if (n == -1) {
        perror("read");
    }

    close(client_sock);
}
