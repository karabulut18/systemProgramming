#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

// Compile with:
// gcc fifo_example.c -o fifo_example -pthread

// The path for our named pipe (FIFO) in the filesystem
#define FIFO_PATH "/tmp/my_test_fifo"
#define MAX_MESSAGES 10
#define MSG_BUFFER_SIZE 100

/**
 * @brief The producer thread. It creates a FIFO, opens it for writing,
 *        sends several messages, and then closes it.
 */
void* producer(void* arg)
{
    int fifo_fd;
    char message_buffer[MSG_BUFFER_SIZE];

    printf("Producer: Waiting to connect to FIFO...\n");

    // Open the FIFO for writing.
    // This call will BLOCK until a reader (the consumer) opens the other end.
    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Producer: Failed to open FIFO for writing");
        return NULL;
    }

    printf("Producer: Consumer connected. Starting to send messages.\n");

    for (int i = 0; i < MAX_MESSAGES; ++i)
    {
        snprintf(message_buffer, MSG_BUFFER_SIZE, "Message #%d from producer", i);
        printf("Producer: Sending -> \"%s\"\n", message_buffer);

        // Write the message to the FIFO.
        // This can also block if the pipe's internal buffer is full.
        if (write(fifo_fd, message_buffer, sizeof(message_buffer)) == -1) {
            perror("Producer: Failed to write to FIFO");
            break;
        }
        usleep(100000); // 100ms delay to make output readable
    }

    printf("Producer: Finished sending messages. Closing FIFO.\n");

    // Closing the write end of the FIFO will cause any readers
    // to receive an end-of-file (read() will return 0).
    close(fifo_fd);

    return NULL;
}

/**
 * @brief The consumer thread. It opens the FIFO for reading,
 *        reads messages until the producer closes its end, and then exits.
 */
void* consumer(void* arg)
{
    int fifo_fd;
    char message_buffer[MSG_BUFFER_SIZE];
    ssize_t bytes_read;

    printf("Consumer: Waiting to connect to FIFO...\n");

    // Open the FIFO for reading.
    // This call will BLOCK until a writer (the producer) opens the other end.
    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Consumer: Failed to open FIFO for reading");
        return NULL;
    }

    printf("Consumer: Connected to FIFO. Waiting for messages.\n");

    // Loop until the producer closes its end of the pipe.
    while ((bytes_read = read(fifo_fd, message_buffer, sizeof(message_buffer))) > 0)
    {
        // Successfully read a message.
        printf("Consumer: Received <- \"%s\"\n", message_buffer);
    }

    if (bytes_read == 0) {
        // End-of-file: The producer has closed its end. This is the expected exit.
        printf("Consumer: Producer closed the pipe. Exiting.\n");
    } else {
        // An error occurred.
        perror("Consumer: Failed to read from FIFO");
    }

    close(fifo_fd);

    // The main thread is responsible for deleting the FIFO file.
    return NULL;
}

int main()
{
    pthread_t prod_thread, cons_thread;

    // Clean up any old FIFO file that might be left over from a crash.
    unlink(FIFO_PATH);

    // Create the FIFO (named pipe) before starting the threads.
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("mkfifo failed");
        return 1;
    }


    printf("Starting Producer and Consumer threads using a FIFO...\n\n");

    // Create the threads. The order doesn't matter due to the blocking nature of open().
    if (pthread_create(&prod_thread, NULL, producer, NULL) != 0)
    {
        perror("Producer thread creation failed");
        unlink(FIFO_PATH);
        return 1;
    }
    if (pthread_create(&cons_thread, NULL, consumer, NULL) != 0) {
        perror("Consumer thread creation failed");
        unlink(FIFO_PATH);
        return 1;
    }

    // Wait for both threads to complete.
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    printf("\nThreads have finished.\n");

    // Remove the FIFO file from the filesystem.
    unlink(FIFO_PATH);

    return 0;
}

