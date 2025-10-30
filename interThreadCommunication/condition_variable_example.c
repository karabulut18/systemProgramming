#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

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

#define MAX_ROUNDS 200

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

    // Initialize mutex and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_not_full, NULL);
    pthread_cond_init(&cond_not_empty, NULL);

    printf("Starting Producer and Consumer threads...\n");

    pthread_create(&prod_thread, NULL, producer, NULL);
    pthread_create(&cons_thread, NULL, consumer, NULL);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    printf("\nThreads have finished.\n");

    // Clean up
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    return 0;
}

