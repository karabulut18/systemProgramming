#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>


#include "common.h"

#define TOTAL_SIZE (100 * 1024 * 1024) // Simulate a 100MB file
#define CHUNK_SIZE (10 * 1024 * 1024)  // Simulate downloading in 10MB chunks

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <fileName>", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fileName = argv[1];

    pid_t my_pid = getpid();
    key_t key;
    int shmid, semid;
    shared_data_t *shared_data;
    struct sembuf pop = {0, -1, SEM_UNDO}; // P operation
    struct sembuf vop = {0, 1, SEM_UNDO};  // V operation

    // Create a file for ftok if it doesn't exist
    FILE* fp = fopen(KEY_PATH, "w");
    if(fp)
        fclose(fp);
    else
    {
        perror("fopen");
        exit(1);
    }


    // 1. Generate a unique key
    key = ftok(KEY_PATH, KEY_ID);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 2. Get or create the semaphore set
    semid = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    int is_first_process = (semid != -1);

    if (is_first_process)
    {
        // We are the first process, so initialize the semaphore to 1 (unlocked)
        printf("Process %d: I am the first. Initializing semaphore.\n", getpid());
        if(semctl(semid, 0, SETVAL, 1) == -1)
        {
            perror("semctl");
            exit(1);
        }
    }
    else if(errno == EEXIST)
    {
        // Semaphore already exists, just get the ID
        semid = semget(key, 1, 0666);
        if (semid == -1)
        {
            perror("semget (existing)");
            exit(1);
        }
    }
    else
    {
        perror("semget (other)");
        exit(1);
    }

    // 3. Get or create the shared memory segment
    shmid = shmget(key, sizeof(shared_data_t), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // 4. Attach the shared memory
    shared_data = shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    if(is_first_process)
    {
        P(semid); // --- LOCK ---
        for(int i = 0; i < MAX_DOWNLOADS; i++)
        {
            shared_data->_slots[i]._status = STATUS_EMPTY;
        }
        V(semid); // --- UNLOCK ---
    }

    printf("Process %d: Wants to download '%s'.\n", my_pid, fileName);
    // Main logic loop
    int slot_index = -1;
    while (1)
    {
        P(semid); // --- LOCK ---

        for(int i = 0; i < MAX_DOWNLOADS; i++)
        {
            download_slot_t *shared_slot = &shared_data->_slots[i];
            if(shared_slot->_status != STATUS_EMPTY
                && strcmp(shared_slot->_file_name, fileName) == 0)
            {
                slot_index = i;
                break;
            }
        }

        if(slot_index != -1)
        {
            download_slot_t *shared_slot = &shared_data->_slots[slot_index];
            if(shared_slot->_status == STATUS_COMPLETED)
            {
                printf("Process %d: File '%s' is already downloaded. Using it.\n", my_pid, fileName);
                V(semid); // --- UNLOCK ---
                break;
            }
            else // Status In Progress
            {
                printf("Process %d: Download of '%s' is in progress by PID %d. Waiting ... %ld%% downloaded\n", my_pid, fileName, shared_slot->_downloader_pid, shared_slot->_bytes_downloaded * 100 / shared_slot->_total_bytes);
                V(semid); // -- UNLOCK --
                sleep(1); // Wait a bit before checking again
                continue; // Go to the start of the while loop
            }
        }
        else
        {
            // find an empty slot
            for(int i = 0; i < MAX_DOWNLOADS; i++)
            {
                download_slot_t* shared_slot = &shared_data->_slots[i];
                if(shared_slot->_status == STATUS_EMPTY)
                {
                    slot_index = i;
                    break;
                }
            }

            if(slot_index == -1)
            {
                printf("Process %d: No empty slots for '%s'. Exiting\n", my_pid, fileName);
                V(semid); // --- UNLOCK ---
                exit(1);
            }
            printf("Process %d: I am the 'chosen one' for '%s'! Starting download.\n", my_pid, fileName);
            download_slot_t* shared_slot = &shared_data->_slots[slot_index];

            shared_slot->_status = STATUS_IN_PROGRESS;
            shared_slot->_downloader_pid = my_pid;
            strncpy(shared_slot->_file_name, fileName, sizeof(shared_slot->_file_name) - 1);
            shared_slot->_total_bytes = TOTAL_SIZE;
            shared_slot->_bytes_downloaded = 0;

            // CRUCIAL: Release the lock before starting the long download
            V(semid); // --- UNLOCK ---

            // --- Simulate a long download in chunks ---
            for (long downloaded_bytes = 0; downloaded_bytes < TOTAL_SIZE; downloaded_bytes += CHUNK_SIZE)
            {
                printf("Process %d: Downloading '%s'... %.0f%%\n", my_pid, fileName, (double)(downloaded_bytes + CHUNK_SIZE) * 100 / TOTAL_SIZE);
                sleep(1); // Simulate work for downloading a chunk

                // Lock, update progress, unlock
                P(semid);
                shared_data->_slots[slot_index]._bytes_downloaded = downloaded_bytes + CHUNK_SIZE;
                V(semid);
            }

            // --- Re-acquire the lock to finalize ---
            printf("Process %d: Download of '%s' finished. Acquiring lock to write to memory...\n", my_pid, fileName);
            P(semid); // --- LOCK ---
            shared_data->_slots[slot_index]._status = STATUS_COMPLETED;
            printf("Process %d: Wrote to shared memory and marked as complete.\n", my_pid);
            V(semid); // --- UNLOCK ---

            break; // Exit loop
        }

    }
    // Now, every process that reaches this point can use the data
    printf("\n--- Process %d is now using the file '%s' ---\n", my_pid, fileName);
    printf("-----------------------------------------\n\n");

    // Detach from shared memory
    if (shmdt(shared_data) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    return 0;
}

