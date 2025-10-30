#ifndef MSG_BUFFER_H
#define MSG_BUFFER_H

#include <sys/types.h>


struct msg_buffer {
    long    _msg_type;
    pid_t   _client_pid;
    char    _msg_text[100];
};

#endif // MSG_BUFFER_H