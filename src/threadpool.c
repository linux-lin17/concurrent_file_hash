#include "threadpool.h"
#include <stdio.h>

// 链表宏定义
#define LIST_INSERT(item, list) do { \
    (item)->prev = NULL; \
    (item)->next = (list); \
    if ((list) != NULL) (list)->prev = (item); \
    (list) = (item); \
} while(0)

#define LIST_REMOVE(item, list) do { \
    if ((item)->prev != NULL) (item)->prev->next = (item)->next; \
    if ((item)->next != NULL) (item)->next->prev = (item)->prev; \
    if ((list) == (item)) (list) = (item)->next; \
    (item)->prev = (item)->next = NULL; \
} while(0)

static void *nThreadPoolCallback(void *arg) {
    struct nWorker *worker = (struct nWorker*)arg;
    while (1) {
        pthread_mutex_lock(&worker->manager->mutex);
        while (worker->manager->tasks == NULL) {
            if (worker->terminate) break;
            pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex);
        }
        if (worker->terminate && worker->manager->tasks == NULL) {
            pthread_mutex_unlock(&worker->manager->mutex);
            break;
        }
        struct nTask *task = worker->manager->tasks;
        if (task) {
            LIST_REMOVE(task, worker->manager->tasks);
        }
        pthread_mutex_unlock(&worker->manager->mutex);

        if (task) {
            task->task_func(task); // 执行真正的计算任务
        }
    }
    return NULL;
}

int nThreadPoolCreate(ThreadPool *pool, int numWorkers) {
    memset(pool, 0, sizeof(ThreadPool));
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    for (int i = 0; i < numWorkers; i++) {
        struct nWorker *worker = (struct nWorker*)malloc(sizeof(struct nWorker));
        memset(worker, 0, sizeof(struct nWorker));
        worker->manager = pool;
        pthread_create(&worker->threadid, NULL, nThreadPoolCallback, worker);
        LIST_INSERT(worker, pool->workers);
    }
    return 0;
}

int nThreadPoolPushTask(ThreadPool *pool, struct nTask *task) {
    pthread_mutex_lock(&pool->mutex);
    LIST_INSERT(task, pool->tasks);
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

int nThreadPoolDestory(ThreadPool *pool) {
    struct nWorker *worker = NULL;
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }
    pthread_cond_broadcast(&pool->cond);
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        pthread_join(worker->threadid, NULL);
    }
    
    return 0;
}