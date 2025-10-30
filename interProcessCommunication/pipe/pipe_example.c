#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 256

int main()
{
    int parent_to_child_pipe[2];
    int child_to_parent_pipe[2];
    pid_t cpid;

    // Create the first pipe (parent writes, child reads)
    if (pipe(parent_to_child_pipe) == -1)
    {
        perror("pipe 1 failed");
        exit(EXIT_FAILURE);
    }

    // Create the second pipe (child writes, parent reads)
    if (pipe(child_to_parent_pipe) == -1)
    {
        perror("pipe 2 failed");
        exit(EXIT_FAILURE);
    }

    cpid = fork();
    if (cpid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0)
    {
        // --- Child Process ---
        // Close unused pipe ends
        close(parent_to_child_pipe[1]); // Child doesn't write to parent->child pipe
        close(child_to_parent_pipe[0]); // Child doesn't read from child->parent pipe

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        // Loop, reading from parent and echoing back
        while ((bytes_read = read(parent_to_child_pipe[0], buffer, BUFFER_SIZE)) > 0)
        {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            printf("Child received: \'%s\'\n", buffer);

            // Echo the message back to the parent
            printf("Child echoing back...\n\n");
            if (write(child_to_parent_pipe[1], buffer, bytes_read) == -1)
            {
                perror("Child: write failed");
                break;
            }
        }

        if (bytes_read == -1) {
            perror("Child: read failed");
        }

        // Clean up and exit
        close(parent_to_child_pipe[0]);
        close(child_to_parent_pipe[1]);
        printf("Child exiting.\n");
        exit(EXIT_SUCCESS);

    }
    else
    { // --- Parent Process ---
        // Close unused pipe ends
        close(parent_to_child_pipe[0]); // Parent doesn't read from parent->child pipe
        close(child_to_parent_pipe[1]); // Parent doesn't write to child->parent pipe

        char buffer[BUFFER_SIZE];

        printf("Parent: Type a message to send to the child (or 'quit' to exit):\n");

        while (1) {
            printf("> ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline

            if (strcmp(buffer, "quit") == 0)
                break;

            // Write message to child
            write(parent_to_child_pipe[1], buffer, strlen(buffer));

            // Read echo from child
            ssize_t bytes_read = read(child_to_parent_pipe[0], buffer, BUFFER_SIZE);
            buffer[bytes_read] = '\0';
            printf("Parent received echo: \'%s\'\n", buffer);
        }

        // Clean up: close pipes to signal EOF to child and wait for it to terminate
        close(parent_to_child_pipe[1]);
        close(child_to_parent_pipe[0]);
        wait(NULL);
        printf("Parent exiting.\n");
        exit(EXIT_SUCCESS);
    }
}