// mq_sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "msg_buffer.h"
#include "constants.h"

// Message buffer structure for System V
// The first member MUST be of type long.


// Global variable for the message queue ID to be accessible by the signal handler
int msgid = -1;

void cleanup_and_exit(int sig)
{
    printf("\nServer: Shutting down and cleaning up message queue...\n");
    if (msgid != -1) {
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("msgctl (cleanup)");
        }
    }
    exit(0);
}

int main()
{
    key_t key;
    struct msg_buffer message;

    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);


    // Create a file to be used by ftok for key generation
    // This ensures both sender and receiver can find the same IPC object.
    FILE* fp = fopen(MSG_KEY_PATH, "w");
    if (fp)
        fclose(fp);

    // 1. Generate a unique key
    key = ftok(MSG_KEY_PATH, MSG_KEY_ID);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 2. Get the message queue ID. Create it if it doesn't exist.
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    printf("Server: Message queue created with ID: %d\n", msgid);
    printf("Server: Waiting for messages... (Press Ctrl+C to shut down)\n\n");

    while (1)
    {
        // 3. Receive any message intended for the server (type 1)
        // The size is now the size of the payload (pid + text)
        if (msgrcv(msgid, &message, sizeof(message) - sizeof(long), MSG_TYPE_SVR, 0) == -1)
        {
            perror("msgrcv");
            break; // Exit loop on error
        }

        printf("Server: Received message from PID %d: \"%s\"\n", message._client_pid, message._msg_text);


        // Send a reply specifically to the client that sent the message
        // The client's PID is used as the message type for the reply.
        message._msg_type = message._client_pid;
        snprintf(message._msg_text, sizeof(message._msg_text), "Acknowledged your message, client %d!", message._client_pid);
        printf("Server: Sending reply to client PID %ld -> \"%s\"\n\n", message._msg_type, message._msg_text);
        
        if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
        }

        usleep(100000);
    }

    cleanup_and_exit(0);
    return 0;
}
