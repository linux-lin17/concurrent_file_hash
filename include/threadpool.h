#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// 任务结构体
struct nTask {
    void (*task_func)(struct nTask *task); // 回调函数
    void *user_data;                      // 传递的数据

    struct nTask *prev;
    struct nTask *next;
};

// 执行者结构体
struct nWorker {
    pthread_t threadid;
    int terminate;
    struct nManager *manager;

    struct nWorker *prev;
    struct nWorker *next;
};

// 线程池管理结构体
typedef struct nManager {
    struct nTask *tasks;
    struct nWorker *workers;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadPool;

// API 声明
int nThreadPoolCreate(ThreadPool *pool, int numWorkers);
int nThreadPoolDestory(ThreadPool *pool);
int nThreadPoolPushTask(ThreadPool *pool, struct nTask *task);

#endif