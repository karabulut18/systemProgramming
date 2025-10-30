#include <stdio.h>
#include <pthread.h>

// To see the race condition, compile without -DUSE_MUTEX
// To see the fix, compile with -DUSE_MUTEX
// gcc mutex_example.c -o mutex_example -pthread -DUSE_MUTEX

// --- Shared Variable ---
// This global variable is shared by all threads.
long long counter = 0;

// --- Mutex ---
// The mutex used to protect the shared variable.
pthread_mutex_t counter_mutex;

#define NUM_INCREMENTS 1000000

/**
 * @brief The function executed by each thread.
 * It increments the global counter NUM_INCREMENTS times.
 */
void* worker_thread_function(void* arg)
{
    for (int i = 0; i < NUM_INCREMENTS; i++)
    {

#ifdef USE_MUTEX
        // Lock the mutex before entering the critical section.
        // If another thread has the lock, this call will block until it's released.
        pthread_mutex_lock(&counter_mutex);
#endif

        // --- CRITICAL SECTION ---
        // Only one thread can be executing this code at a time when the mutex is used.
        counter++;
        // --- END CRITICAL SECTION ---

#ifdef USE_MUTEX
        // Unlock the mutex, allowing other waiting threads to proceed.
        pthread_mutex_unlock(&counter_mutex);
#endif
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    // Initialize the mutex. This must be done before it's used.
    if (pthread_mutex_init(&counter_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
        return 1;
    }

    printf("Starting threads...\n");

    // Create two threads, both running the same worker function.
    pthread_create(&thread1, NULL, worker_thread_function, NULL);
    pthread_create(&thread2, NULL, worker_thread_function, NULL);

    // Wait for both threads to complete their execution.
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Threads have finished.\n");

    // Destroy the mutex to free up any resources.
    pthread_mutex_destroy(&counter_mutex);

    // Print the final result.
    printf("Expected counter value: %d\n", NUM_INCREMENTS * 2);
    printf("Actual counter value:   %lld\n", counter);

    printf("%s\n", (counter == NUM_INCREMENTS * 2) ? "Success!" : "Failure!");

    return 0;
}

