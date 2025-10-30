#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

// Compile with:
// gcc condition_variable_example.c -o condition_variable_example -pthread

// --- Shared Buffer and State ---
#define BUFFER_SIZE 30
int buffer[BUFFER_SIZE];
int count = 0; // Number of items currently in the buffer
int in_index = 0;  // Index where producer will add the next item
int out_index = 0; // Index where consumer will take the next item

// --- Synchronization Primitives ---
pthread_mutex_t mutex;
pthread_cond_t cond_not_full;  // Signaled when the buffer is no longer full
pthread_cond_t cond_not_empty; // Signaled when the buffer is no longer empty

#define MAX_ROUNDS 50

/**
 * @brief Produces items and puts them into the buffer.
 */
void* producer(void* arg)
{
    for (int i = 0; i < MAX_ROUNDS; ++i)
    {
        int item = i * 10; // Produce an item

        pthread_mutex_lock(&mutex);

        // If the buffer is full, wait for the consumer to make space.
        // The 'while' is crucial to handle "spurious wakeups".
        while (count == BUFFER_SIZE)
        {
            printf("Producer: Buffer is FULL. Waiting...\n");
            // pthread_cond_wait atomically unlocks the mutex and puts the thread to sleep.
            // When it wakes up, it re-acquires the lock before continuing.
            pthread_cond_wait(&cond_not_full, &mutex);
        }

        // Add item to the buffer
        buffer[in_index] = item;
        in_index = (in_index + 1) % BUFFER_SIZE;
        count++;
        printf("Producer: Produced item %d, buffer count is now %d\n", item, count);

        // Signal to a waiting consumer that the buffer is no longer empty.
        pthread_cond_signal(&cond_not_empty);

        pthread_mutex_unlock(&mutex);

        //usleep(50000); // Simulate some work
    }
    return NULL;
}

/**
 * @brief Consumes items from the buffer.
 */
void* consumer(void* arg)
{
    for (int i = 0; i < MAX_ROUNDS; ++i)
    {
        pthread_mutex_lock(&mutex);

        // If the buffer is empty, wait for the producer to add something.
        while (count == 0) {
            printf("Consumer: Buffer is EMPTY. Waiting...\n");
            pthread_cond_wait(&cond_not_empty, &mutex);
        }

        // Remove item from the buffer
        int item = buffer[out_index];
        out_index = (out_index + 1) % BUFFER_SIZE;
        count--;
        printf("Consumer: Consumed item %d, buffer count is now %d\n", item, count);

        // Signal to a waiting producer that the buffer is no longer full.
        pthread_cond_signal(&cond_not_full);

        pthread_mutex_unlock(&mutex);

        //usleep(200000); // Simulate more work to allow the buffer to fill up
    }
    return NULL;
}

int main()
{
    pthread_t prod_thread, cons_thread;
    pthread_attr_t prod_attr, cons_attr;
    struct sched_param prod_param, cons_param;


    // Initialize mutex and condition variables
    if (pthread_mutex_init(&mutex, NULL) != 0) { perror("Mutex init failed"); return 1; }
    if (pthread_cond_init(&cond_not_full, NULL) != 0) { perror("Cond (not_full) init failed"); return 1; }
    if (pthread_cond_init(&cond_not_empty, NULL) != 0) { perror("Cond (not_empty) init failed"); return 1; }

        // --- Priority Setup ---
    if (pthread_attr_init(&prod_attr) != 0) { perror("Producer attr init failed"); return 1; }
    if (pthread_attr_init(&cons_attr) != 0) { perror("Consumer attr init failed"); return 1; }


    // Set threads to use the scheduling policy and priority from the attributes.
    pthread_attr_setinheritsched(&prod_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&cons_attr, PTHREAD_EXPLICIT_SCHED);

    // Set the scheduling policy to Round-Robin for both.
    pthread_attr_setschedpolicy(&prod_attr, SCHED_RR);
    pthread_attr_setschedpolicy(&cons_attr, SCHED_RR);

    // Get the valid priority range for the SCHED_RR policy.
    int max_prio = sched_get_priority_max(SCHED_RR);
    int min_prio = sched_get_priority_min(SCHED_RR);
    printf("SCHED_RR Priority Range: Min=%d, Max=%d\n", min_prio, max_prio);
    printf("NOTE: Must run with sudo for priorities to take effect.\n\n");

    // Set the producer thread to the maximum priority.
    prod_param.sched_priority = max_prio;
    pthread_attr_setschedparam(&prod_attr, &prod_param);

    // Set the consumer thread to the minimum priority.
    cons_param.sched_priority = min_prio;
    pthread_attr_setschedparam(&cons_attr, &cons_param);

    printf("Starting Producer (High Prio) and Consumer (Low Prio) threads...\n");

    // Create threads with their respective attributes
    if (pthread_create(&prod_thread, &prod_attr, producer, NULL) != 0) { perror("Producer create failed"); return 1; }
    if (pthread_create(&cons_thread, &cons_attr, consumer, NULL) != 0) { perror("Consumer create failed"); return 1; }


    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    printf("\nThreads have finished.\n");

    // Clean up
    pthread_attr_destroy(&prod_attr);
    pthread_attr_destroy(&cons_attr);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    return 0;
}

