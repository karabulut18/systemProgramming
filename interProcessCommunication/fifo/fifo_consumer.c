#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "./fifo_constants.h"


// Compile with: gcc fifo_consumer.c -o consumer


int main() {
    int fifo_fd;
    char message_buffer[MSG_BUFFER_SIZE];
    ssize_t bytes_read;

    printf("Consumer: Waiting for the FIFO to be created...\n");

    // Open the FIFO for reading.
    // This call will BLOCK until a writer (the producer) opens the other end.
    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Consumer: Failed to open FIFO for reading");
        return 1;
    }

    printf("Consumer: Connected to FIFO. Waiting for messages.\n");

    // Loop until the producer closes its end of the pipe.
    // read() will return 0 when the write-end of the pipe is closed.
    while ((bytes_read = read(fifo_fd, message_buffer, sizeof(message_buffer))) > 0) {
        printf("Consumer: Received <- \"%s\"\n", message_buffer);
    }

    if (bytes_read == 0) {
        // End-of-file: The producer has closed its end. This is the expected exit.
        printf("Consumer: Producer closed the pipe. Exiting.\n");
    } else {
        // An error occurred.
        perror("Consumer: Read error");
    }

    close(fifo_fd);

    // The consumer cleans up the FIFO file from the filesystem.
    unlink(FIFO_PATH);

    return 0;
}
