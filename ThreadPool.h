#ifndef THREAD_POOL_H 
#define THREAD_POOL_H

#include <stdlib.h>

typedef struct thread_pool {
  u_int32_t max_threads;
  u_int32_t capacity;
  u_int32_t q_size;
  struct thread_pool_job *job_q;
  pthread_mutex_t q_lock;
} thread_pool_t;

void thread_pool_init(thread_pool_t *tpool, u_int32_t max_threads, u_int32_t capacity);
bool thread_pool_submit_job(thread_pool_t *tpool, void *(* job)(void *), void *args);
void thread_pool_run_and_wait(thread_pool_t *tpool);
void thread_pool_destroy(thread_pool_t *tpool);

#endif