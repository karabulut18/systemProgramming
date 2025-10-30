#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>


#include "msg_buffer.h"
#include "constants.h"


// Message buffer structure for System V
// The first member MUST be of type long.

void cleanup_and_exit(int sig)
{
    printf("\nClient: Shutting down and cleaning up message queue...\n");
    exit(0);
}


int main() {
    key_t key;
    struct msg_buffer message;
    int msgid;
    int my_pid = getpid();

    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);


    // 1. Generate the same unique key as the sender
    key = ftok(MSG_KEY_PATH, MSG_KEY_ID);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 2. Get the message queue ID.
    msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }
    printf("Client (PID %d): Message queue ID: %d\n", my_pid, msgid);
    printf("Type a message and press Enter to send. Press Ctrl+C to exit.\n");

    while(1)
    {
        printf("> ");
        if(fgets(message._msg_text, sizeof(message._msg_text), stdin) == NULL)
            break;

        // Remove the newline character from the input
        message._msg_text[strcspn(message._msg_text, "\n")] = 0;

        message._msg_type = MSG_TYPE_SVR;
        message._client_pid = my_pid;

        if(msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
            break;
        }
        // 4. receive a reply specifically for us (type = our pid)
        if(msgrcv(msgid, &message, sizeof(message) - sizeof(long), my_pid, 0) == -1)
        {
            perror("msgrcv");
            break;
        }
        printf("Client (PID %d): Received reply: \"%s\"\n", my_pid, message._msg_text);
        usleep(100000);
    }

    printf("Client (PID %d): Shutting down and cleaning up message queue...\n", my_pid);


    return 0;
};