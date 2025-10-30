#include <stdio.h>
#include <pthread.h>
#include <sched.h> // For scheduling policies and priorities
#include <unistd.h> // For sysconf

// NOTE: To see the effect of priorities, you must compile and run with sudo:
// gcc priority_example.c -o priority_example -pthread
// sudo ./priority_example

// --- Shared Variable ---
long long shared_counter = 0;
pthread_mutex_t counter_mutex;

#define TOTAL_INCREMENTS 2000000

// A struct to pass data to and from the thread.
typedef struct
{
    int thread_id;
    long long increments_done;
} thread_data_t;

/**
 * @brief The function executed by each thread.
 * It increments the shared counter until it reaches the total,
 * and counts how many increments it performed itself.
 */
void* worker_thread_function(void* arg)
{
    thread_data_t* data = (thread_data_t*)arg;
    data->increments_done = 0;

    printf("Thread %d starting.\n", data->thread_id);

    while (1)
    {
        // Lock the mutex to check and update the shared counter.
        pthread_mutex_lock(&counter_mutex);

        if (shared_counter >= TOTAL_INCREMENTS)
        {
            // The goal has been reached, unlock and exit the loop.
            pthread_mutex_unlock(&counter_mutex);
            break;
        }

        // --- CRITICAL SECTION ---
        shared_counter++;
        data->increments_done++;
        // --- END CRITICAL SECTION ---

        pthread_mutex_unlock(&counter_mutex);
    }

    printf("Thread %d finished.\n", data->thread_id);
    return NULL;
}

int main()
{
    pthread_t high_prio_thread, low_prio_thread;
    thread_data_t high_prio_data = { .thread_id = 1 };
    thread_data_t low_prio_data = { .thread_id = 2 };

    // --- Attribute and Priority Setup ---
    pthread_attr_t high_prio_attr, low_prio_attr;
    struct sched_param high_prio_param, low_prio_param;

    // Initialize attributes and mutex
    pthread_attr_init(&high_prio_attr);
    pthread_attr_init(&low_prio_attr);
    pthread_mutex_init(&counter_mutex, NULL);

    // Set the threads to use the scheduling policy and priority from the attributes.
    pthread_attr_setinheritsched(&high_prio_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&low_prio_attr, PTHREAD_EXPLICIT_SCHED);

    // Set the scheduling policy to Round-Robin for both.
    pthread_attr_setschedpolicy(&high_prio_attr, SCHED_RR);
    pthread_attr_setschedpolicy(&low_prio_attr, SCHED_RR);

    // Get the valid priority range for the SCHED_RR policy.
    int max_prio = sched_get_priority_max(SCHED_RR);
    int min_prio = sched_get_priority_min(SCHED_RR);
    printf("Priority range for SCHED_RR: Min=%d, Max=%d\n", min_prio, max_prio);
    printf("Running with sudo is required for priorities to take effect.\n\n");

    // Set the high-priority thread to the maximum priority.
    high_prio_param.sched_priority = max_prio;
    pthread_attr_setschedparam(&high_prio_attr, &high_prio_param);

    // Set the low-priority thread to the minimum priority.
    low_prio_param.sched_priority = min_prio;
    pthread_attr_setschedparam(&low_prio_attr, &low_prio_param);

    // --- Thread Creation ---
    printf("Starting threads...\n");
    pthread_create(&high_prio_thread, &high_prio_attr, worker_thread_function, &high_prio_data);
    pthread_create(&low_prio_thread, &low_prio_attr, worker_thread_function, &low_prio_data);

    // --- Wait and Report ---
    pthread_join(high_prio_thread, NULL);
    pthread_join(low_prio_thread, NULL);

    printf("\n--- Results ---\n");
    printf("Total increments performed: %lld\n", shared_counter);
    printf("Thread 1 (High Prio) did %lld increments.\n", high_prio_data.increments_done);
    printf("Thread 2 (Low Prio)  did %lld increments.\n", low_prio_data.increments_done);
    printf("Total increments accounted for: %lld\n", high_prio_data.increments_done + low_prio_data.increments_done);

    if (high_prio_data.increments_done > low_prio_data.increments_done)
        printf("\nConclusion: The high-priority thread did more work, as expected.\n");
    else
        printf("\nConclusion: The low-priority thread did more work. This can happen if run without sudo or on a very busy system.\n");

    // --- Cleanup ---
    pthread_attr_destroy(&high_prio_attr);
    pthread_attr_destroy(&low_prio_attr);
    pthread_mutex_destroy(&counter_mutex);

    return 0;
}