#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h> // For O_CREAT constants
#include <unistd.h> // For usleep

// Compile with:
// gcc semaphore_handoff_example.c -o semaphore_handoff_example -pthread

// --- Shared Variable ---
long long shared_counter = 0;

// --- Semaphores ---
sem_t* sem_non_sevens_turn;
sem_t* sem_sevens_turn;

// --- Constants ---
#define NON_SEVENS_SEM_NAME "/non_sevens_turn"
#define SEVENS_SEM_NAME "/sevens_turn"

#define TOTAL_INCREMENTS 30

/**
 * @brief This thread increments the counter as long as the result is NOT a multiple of 7.
 */
void* worker_non_sevens(void* arg)
{
    while (1)
    {
        // Wait for our turn. This will block until the other thread posts to our semaphore.
        sem_wait(sem_non_sevens_turn);

        // Check for termination condition after waking up.
        if (shared_counter >= TOTAL_INCREMENTS)
        {
            sem_post(sem_sevens_turn); // Wake up the other thread so it can terminate too.
            break;
        }

        // Check if the *next* number will be a multiple of 7.
        if ((shared_counter + 1) % 7 == 0)
        {
            // It is. Our work is done for now. Hand off to the other thread.
            printf("Non-7s Thread: Reached %lld, handing off to 7s thread.\n", shared_counter);
            sem_post(sem_sevens_turn);
        }
        else
        {
            // It's still our turn. Post to our own semaphore to continue immediately.
            shared_counter++;
            printf("Non-7s Thread: Incremented to %lld\n", shared_counter);
            sem_post(sem_non_sevens_turn);
        }
        usleep(50000); // Sleep for 50ms to make the output readable
    }
    return NULL;
}

/**
 * @brief This thread's only job is to perform the increment that results in a multiple of 7.
 */
void* worker_sevens(void* arg)
{
    while (1)
    {
        // Wait for our turn. This will block until the non-sevens thread hands off.
        sem_wait(sem_sevens_turn);

        if (shared_counter >= TOTAL_INCREMENTS)
        {
            sem_post(sem_non_sevens_turn); // Wake up the other thread so it can terminate.
            break;
        }

        // Our job is to increment to the multiple of 7.
        shared_counter++;
        printf("   7s Thread: Incremented to %lld <--- Multiple of 7!\n", shared_counter);

        // Our work is done. Hand control back to the other thread.
        sem_post(sem_non_sevens_turn);
        usleep(50000); // Sleep for 50ms
    }
    return NULL;
}

int main()
{
    pthread_t thread_non_sevens, thread_sevens;

    // Unlink any previous instances of the semaphores, in case the program crashed.
    sem_unlink(NON_SEVENS_SEM_NAME);
    sem_unlink(SEVENS_SEM_NAME);

    // Open named semaphores (modern, portable approach).
    // O_CREAT: Create the semaphore if it doesn't exist.
    // 0644: Permissions for the semaphore file.
    // Initial value: 1 for the first thread, 0 for the second.
    sem_non_sevens_turn = sem_open(NON_SEVENS_SEM_NAME, O_CREAT, 0644, 1);
    sem_sevens_turn = sem_open(SEVENS_SEM_NAME, O_CREAT, 0644, 0);

    printf("Starting threads to count to %d...\n", TOTAL_INCREMENTS);

    pthread_create(&thread_non_sevens, NULL, worker_non_sevens, NULL);
    pthread_create(&thread_sevens, NULL, worker_sevens, NULL);

    pthread_join(thread_non_sevens, NULL);
    pthread_join(thread_sevens, NULL);

    printf("\nFinished. Final counter value: %lld\n", shared_counter);

    // Clean up the semaphores.
    // Close the semaphores.
    sem_close(sem_non_sevens_turn);
    sem_close(sem_sevens_turn);
    // Unlink them from the system to clean up.
    sem_unlink(NON_SEVENS_SEM_NAME);
    sem_unlink(SEVENS_SEM_NAME);

    return 0;
}