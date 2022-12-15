#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include "ThreadPool.h"

typedef struct thread_pool_job {
  void *(* job)(void *);
  void *args;
} thread_pool_job_t;

static void *thread_pool_thread_init(void *vtpool);
static bool thread_pool_push_job(thread_pool_t *tpool, thread_pool_job_t job);
static thread_pool_job_t *thread_pool_pop_job(thread_pool_t *tpool);

/* Initialises a thread pool with the provided max_threads and job capacity. */
void thread_pool_init(thread_pool_t *tpool, u_int32_t max_threads, u_int32_t capacity) {
  tpool->max_threads = max_threads;
  tpool->capacity = capacity;
  tpool->q_size = 0;
  tpool->job_q = malloc(capacity * sizeof(thread_pool_job_t));
  pthread_mutex_init(&tpool->q_lock, NULL);
}

/* Submits a job to the thread_pool with provided args. 
   Returns whether submission was successful. */
bool thread_pool_submit_job(thread_pool_t *tpool, void *(* job)(void *), void *args) {
  thread_pool_job_t j = { job, args };
  bool pushed = thread_pool_push_job(tpool, j);
  if (!pushed)
    perror("Capacity of thread pool has been exceeded.");

  return pushed;
}

/* Creates all the threads possible for the thread pool and finishes all jobs
   in the job queue. */
void thread_pool_run_and_wait(thread_pool_t *tpool) {
  pthread_t *threads = malloc(tpool->max_threads * sizeof(pthread_t));

  // Creates and stores number of threads that the thread pool supports.
  for (int i = 0; i < tpool->max_threads; i++)
    pthread_create(&threads[i], NULL, &thread_pool_thread_init, tpool);
    
  // Waits for all the threads to finish.
  for (int i = 0; i < tpool->max_threads; i++)
    pthread_join(threads[i], NULL);
   
  free(threads);
}

/* Frees resources used by a thread pool. */
void thread_pool_destroy(thread_pool_t *tpool) {
  free(tpool->job_q);
}

/* Wrapper around jobs for thread pool threads. 
   This allows for threads to be preserved in between jobs. */
static void *thread_pool_thread_init(void *vtpool) {
  thread_pool_t *tpool = (thread_pool_t *) vtpool;
  thread_pool_job_t *tpool_job = thread_pool_pop_job(tpool);

  while (tpool_job != NULL) {
    tpool_job->job(tpool_job->args);
    tpool_job = thread_pool_pop_job(tpool);
  }

  return NULL;
}

/* Atomically pushes a job into the thread pool's queue. 
   Returns whether the push was successful (if the capacity wasn't exceeded). */
static bool thread_pool_push_job(thread_pool_t *tpool, thread_pool_job_t job) {
  pthread_mutex_lock(&tpool->q_lock);

  if (tpool->q_size == tpool->capacity) {
    pthread_mutex_unlock(&tpool->q_lock);
    return false;
  }

  tpool->job_q[tpool->q_size] = job;
  tpool->q_size++;

  pthread_mutex_unlock(&tpool->q_lock);
  return true;
}

/* Atomically pops a job out of the thread pool's queue.
   If there are no jobs, then this returns null. Otherwise, a pointer
   to the popped job is returned. */
static thread_pool_job_t *thread_pool_pop_job(thread_pool_t *tpool) {
  pthread_mutex_lock(&tpool->q_lock);

  if (tpool->q_size == 0) {
    pthread_mutex_unlock(&tpool->q_lock);
    return NULL;
  }

  tpool->q_size--;
  thread_pool_job_t *job = tpool->job_q + tpool->q_size;

  pthread_mutex_unlock(&tpool->q_lock);
  return job;
}