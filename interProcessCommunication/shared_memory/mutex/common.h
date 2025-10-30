#ifndef DOWNLOAD_COMMON_H
#define DOWNLOAD_COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>

// Define a key for ftok() to find the shared memory
#define KEY_PATH "downloader_key_file"
#define KEY_ID 'M'

#define MAX_DOWNLOADS 10

typedef enum
{
    STATUS_EMPTY = 0,
    STATUS_IN_PROGRESS,
    STATUS_COMPLETED
} download_status_t;

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
    pthread_mutex_t _mutex;
    download_slot_t _slots[MAX_DOWNLOADS];
} shared_data_t;

#endif // DOWNLOAD_COMMON_H