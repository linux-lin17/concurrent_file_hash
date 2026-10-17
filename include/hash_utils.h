#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include "threadpool.h" 

struct FileData {
    char path[512];
};

// 全局变量声明
extern int g_fast_mode;   // 快速模式开关
extern int g_total_files; // 文件总数计数器

void compute_hash_func(struct nTask *task);
void scan_dir(const char *dir_path, ThreadPool *pool);

#endif