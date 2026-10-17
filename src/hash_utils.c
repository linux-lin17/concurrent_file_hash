#define _GNU_SOURCE
#include "hash_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <openssl/md5.h> // 确保包含 MD5

// 定义全局变量
int g_fast_mode = 0;
int g_total_files = 0; // 初始化计数器

// 辅助函数：把二进制转成 16 进制字符串
void bin2hex(unsigned char *src, int len, char *dest) {
    for (int i = 0; i < len; i++) {
        sprintf(dest + i * 2, "%02x", src[i]);
    }
}

void compute_hash_func(struct nTask *task) {
    // 支持 MD5/SHA1/SHA256
    struct FileData *data = (struct FileData *)task->user_data;
    
    FILE *fp = fopen(data->path, "rb");
    if (!fp) { free(data); free(task); return; }

    SHA256_CTX ctx256; SHA256_Init(&ctx256);
    SHA_CTX ctx1;      SHA1_Init(&ctx1);
    MD5_CTX ctx5;      MD5_Init(&ctx5);

    unsigned char buf[4096];
    size_t n;
    size_t total_read = 0;

    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        SHA256_Update(&ctx256, buf, n);
        SHA1_Update(&ctx1, buf, n);
        MD5_Update(&ctx5, buf, n);
        total_read += n;
        if (g_fast_mode && total_read >= 1024 * 1024) break;
    }
    
    unsigned char hash256[SHA256_DIGEST_LENGTH];
    unsigned char hash1[SHA_DIGEST_LENGTH];
    unsigned char hash5[MD5_DIGEST_LENGTH];

    SHA256_Final(hash256, &ctx256);
    SHA1_Final(hash1, &ctx1);
    MD5_Final(hash5, &ctx5);

    fclose(fp);

    char str256[65], str1[41], str5[33];
    bin2hex(hash256, SHA256_DIGEST_LENGTH, str256);
    bin2hex(hash1, SHA_DIGEST_LENGTH, str1);
    bin2hex(hash5, MD5_DIGEST_LENGTH, str5);

    // CSV 格式输出
    char output[2048];
    snprintf(output, sizeof(output), "%s,%s,%s,%s\n", data->path, str5, str1, str256);
    printf("%s", output);

    free(data);
    free(task);
}


void scan_dir(const char *dir_path, ThreadPool *pool) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            scan_dir(full_path, pool); 
        } else if (entry->d_type == DT_REG) {
            
            // 每发现一个文件，计数器加 1
            g_total_files++; 

            struct nTask *task = (struct nTask*)malloc(sizeof(struct nTask));
            struct FileData *data = (struct FileData*)malloc(sizeof(struct FileData));
            strcpy(data->path, full_path);

            task->task_func = compute_hash_func;
            task->user_data = data;
            
            nThreadPoolPushTask(pool, task);
        }
    }
    closedir(dir);
}