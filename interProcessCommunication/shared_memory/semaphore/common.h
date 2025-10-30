#ifndef DOWNLOAD_COMMON_H
#define DOWNLOAD_COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// Define a key for ftok() to find the shared memory and semaphore
#define KEY_PATH "downloader_key_file"
#define KEY_ID 'D'

// A simple semaphore lock/unlock macro set
#define P(s) semop(s, &pop, 1) // P operation (wait/lock)
#define V(s) semop(s, &vop, 1) // V operation (signal/unlock)

#define MAX_DOWNLOADS 10

typedef enum
{
    STATUS_EMPTY = 0,
    STATUS_IN_PROGRESS,
    STATUS_COMPLETED
} download_status_t;

#define FILE_CONTENT_SIZE 1024
#define FILE_NAME_SIZE 256

// The structure that will be placed in shared memory
typedef struct {
    download_status_t _status;
    pid_t _downloader_pid;
    char _file_name[FILE_NAME_SIZE];
    long _bytes_downloaded;
    long _total_bytes;
} download_slot_t;

typedef struct 
{
    download_slot_t _slots[MAX_DOWNLOADS];
} shared_data_t;

#endif // DOWNLOAD_COMMON_H