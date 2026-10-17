#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h> //为了用 tolower
#include "threadpool.h"
#include "hash_utils.h"

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL);
    return te.tv_sec*1000LL + te.tv_usec/1000;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dir>\n", argv[0]);
        return -1;
    }

    // 1. 【交互式界面】询问是否开启快速模式
    fprintf(stderr, ">>> Enable Fast Mode (calculate first 1MB only)? [y/N]: ");
    
    char answer = 0;

    // 读取用户输入的一个字符
    scanf(" %c", &answer); // 吃掉可能的回车符

    if (answer == 'y' || answer == 'Y') {
        g_fast_mode = 1;
        fprintf(stderr, ">>> [Status] Fast Mode ENABLED.\n");
    } else {
        g_fast_mode = 0;
        fprintf(stderr, ">>> [Status] Fast Mode DISABLED (Full Calculation).\n");
    }

    ThreadPool pool;
    nThreadPoolCreate(&pool, 8); 

    // 输出 CSV 表头 (到 stdout，进入文件)
    printf("FilePath,MD5,SHA1,SHA256\n");

    long long start_time = current_timestamp();

    fprintf(stderr, ">>> Scanning %s ...\n", argv[1]);
    scan_dir(argv[1], &pool);

    // 销毁池子
    nThreadPoolDestory(&pool);

    long long end_time = current_timestamp();
    double total_seconds = (end_time - start_time) / 1000.0;
    
    double avg_time = 0.0;
    if (g_total_files > 0) {
        avg_time = total_seconds / g_total_files;
    }

    // ---------------------------------------------------------
    // 1. 【屏幕看】打印到 stderr (不会进文件)
    // ---------------------------------------------------------
    fprintf(stderr, "\n================ REPORT ================\n");
    fprintf(stderr, " Total Files   : %d\n", g_total_files);
    fprintf(stderr, " Total Time    : %.3f s\n", total_seconds);
    fprintf(stderr, " Avg Time/File : %.3f ms\n", avg_time * 1000);
    fprintf(stderr, "========================================\n");
    fprintf(stderr, ">>> Result saved to stdout (or file if redirected).\n");

    // ---------------------------------------------------------
    // 2. 【文件看】打印到 stdout (会进 CSV 文件)
    // ---------------------------------------------------------
    printf("\n");
    printf("--- Statistics ---\n");
    printf("Metric,Value\n");       // 统计表的表头
    printf("Total Files,%d\n", g_total_files);
    printf("Total Time (s),%.3f\n", total_seconds);
    printf("Avg Time per File (ms),%.3f\n", avg_time * 1000);

    return 0;
}