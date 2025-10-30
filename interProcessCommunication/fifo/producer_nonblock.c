// producer_nonblock.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "./fifo_constants.h"

int main() 
{
    int fifo_fd;
    char message_buffer[100];
    int messages_sent = 0;

    // Create the FIFO (named pipe).
    // The 0666 permissions allow any user to read/write.
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        // If the error is EEXIST, it means the file already exists, which is fine.
        // Any other error is a problem.
        if (errno != EEXIST) {
            perror("mkfifo");
            return 1;
        }
    }

    printf("Producer: Opening FIFO for writing...\n");

    // Open the FIFO for both reading and writing.
    // This is a common trick to prevent the open() call from blocking
    // if there are no readers yet. The process itself satisfies the
    // "at least one reader" requirement.
    fifo_fd = open(FIFO_PATH, O_RDWR);
    if (fifo_fd == -1) {
        perror("Producer: Failed to open FIFO");
        return 1;
    }

    printf("Producer: FIFO opened. Starting to send messages.\n");

    for (int i = 0; i < MAX_MESSAGES; ++i)
    {
        snprintf(message_buffer, sizeof(message_buffer), "Message #%d", i);
        
        // The write() call will block if the pipe's buffer is full.
        // This is the kernel's flow control mechanism.
        if (write(fifo_fd, message_buffer, sizeof(message_buffer)) == -1)
        {
            perror("Producer: write error");
            break;
        }
        else
            printf("Producer: Sent message: %s\n", message_buffer);

        messages_sent++;
    }

    printf("Producer: Finished sending %d messages. Closing FIFO.\n", messages_sent);
    close(fifo_fd);

    return 0;
}
