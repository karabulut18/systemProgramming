#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <unistd.h>


#include "common.h"

int main() {
    key_t key;
    int shmid, semid;

    // Create the key file if it doesn't exist, so ftok doesn't fail
    FILE* fp = fopen(KEY_PATH, "a");
    if (fp) fclose(fp);

    // 1. Generate the same unique key
    key = ftok(KEY_PATH, KEY_ID);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // 2. Get the ID of the shared memory segment
    shmid = shmget(key, 0, 0);
    if (shmid != -1) {
        // Remove the shared memory segment
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
        } else {
            printf("Shared memory segment removed.\n");
        }
    } else if (errno != ENOENT) {
        perror("shmget for cleanup");
    }

    // 3. Get the ID of the semaphore set
    semid = semget(key, 0, 0);
    if (semid != -1) {
        // Remove the semaphore set
        if (semctl(semid, 0, IPC_RMID) == -1) {
            perror("semctl");
        } else {
            printf("Semaphore set removed.\n");
        }
    } else if (errno != ENOENT) {
        perror("semget for cleanup");
    }

    // Remove the key file
    unlink(KEY_PATH);

    printf("Cleanup complete.\n");
    return 0;
}
