// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "constants.h"

int main()
{
    int client_sock;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create a socket
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Set up the server address
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. Connect to the server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type 'exit' to quit.\n");

    // 4. Communication loop
    while (1)
    {
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0)
            break;

        // Send message to server
        if (write(client_sock, buffer, strlen(buffer)) < 0)
        {
            perror("write error");
            break;
        }

        // Read response from server
        int n = read(client_sock, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Server replied: %s\n", buffer);
        } else if (n == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            perror("read error");
            break;
        }
    }

    close(client_sock);
    return 0;
}
