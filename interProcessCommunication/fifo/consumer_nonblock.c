// consumer_nonblock.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "./fifo_constants.h"

int main() {
    int fifo_fd;
    char message_buffer[MSG_BUFFER_SIZE];
    ssize_t bytes_read;
    int messages_received = 0;

    printf("Consumer: Waiting to open FIFO for reading...\n");

    // Open the FIFO for reading. This will block until a writer opens it.
    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Consumer: Failed to open FIFO for reading");
        return 1;
    }

    printf("Consumer: Connected to FIFO. Waiting for messages.\n");

    // read() will return 0 when the writer closes the pipe.
    while ((bytes_read = read(fifo_fd, message_buffer, sizeof(message_buffer))) > 0) {
        printf("Consumer: Received <- \"%s\"\n", message_buffer);
        messages_received++;
    }

    if (bytes_read == 0) {
        printf("Consumer: Producer closed the pipe. Received %d messages. Exiting.\n", messages_received);
    } else {
        perror("Consumer: Read error");
    }

    close(fifo_fd);
    unlink(FIFO_PATH); // The consumer cleans up the FIFO file.

    return 0;
}
