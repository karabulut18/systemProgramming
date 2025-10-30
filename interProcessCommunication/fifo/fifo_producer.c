
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "./fifo_constants.h"

// Compile with: gcc fifo_producer.c -o producer


int main()
{
    int fifo_fd;
    char message_buffer[MSG_BUFFER_SIZE];

    // Create the FIFO (named pipe).
    // mkfifo returns 0 on success, -1 on error.
    // We ignore EEXIST error, which means the file already exists.
    if ((mkfifo(FIFO_PATH, 0666) == -1)) {
        perror("mkfifo");
        // If the file already exists, we can continue.
        // For other errors, we should exit.
    }

    printf("Producer: Waiting for a consumer to connect...\n");

    // Open the FIFO for writing.
    // This call will BLOCK until a reader (the consumer) opens the other end.
    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Producer: Failed to open FIFO for writing");
        return 1;
    }

    printf("Producer: Consumer connected. Sending messages.\n");

    for (int i = 0; i < MAX_MESSAGES; ++i) {
        snprintf(message_buffer, MSG_BUFFER_SIZE, "Message #%d from producer", i);
        printf("Producer: Sending -> \"%s\"\n", message_buffer);
        write(fifo_fd, message_buffer, sizeof(message_buffer));
        sleep(1); // Sleep for a second to make the output easy to follow
    }

    printf("Producer: Finished sending messages. Closing FIFO.\n");
    close(fifo_fd);

    return 0;
}

