# CHashProject — 并发文件哈希计算工具

![C99](https://img.shields.io/badge/language-C99-blue)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![Build](https://img.shields.io/badge/build-CMake-green)

基于 **pthread 从零手写线程池** 的并发文件哈希工具：递归扫描目录，多线程并行计算文件的 **MD5 / SHA1 / SHA256**，输出 CSV 结果与性能统计。

## 功能特性

- **手写线程池**：生产者-消费者模型，固定 8 个 worker 线程，互斥锁 + 条件变量同步
- **三哈希并行计算**：单次 4KB 分块读取，同时更新三路哈希上下文，一次 I/O 产出三个结果
- **快速模式**：仅计算文件前 1MB，适配超大文件的快速扫描场景
- **递归目录扫描**：自动遍历所有子目录
- **双通道输出**：CSV 数据走 stdout（可重定向为文件），运行报告走 stderr（不污染数据）
- **性能统计**：总文件数、总耗时、平均每文件耗时

## 技术栈

| 类别 | 技术 |
|---|---|
| 语言 | C99 |
| 并发 | POSIX pthread、线程池、生产者-消费者模型 |
| 同步 | `pthread_mutex_t`、`pthread_cond_t` |
| 数据结构 | 侵入式双向链表（O(1) 任务队列） |
| 哈希算法 | OpenSSL（MD5 / SHA1 / SHA256） |
| 构建 | CMake ≥ 3.10 |

## 架构设计

```
                    ┌─────────────────────────────────────┐
   scan_dir()       │            ThreadPool               │
 (生产者/主线程)     │                                     │
                    │   tasks ───► 侵入式双向链表队列       │
 每发现一个文件      │     ▲                             │
 创建 nTask ────────┼─────┘        mutex + cond 保护      │
 (任务入队+signal)   │                                     │
                    │  workers ──► 8 个 worker 线程        │
                    │    │  循环: 等任务→取任务→锁外计算    │
                    │    └──► compute_hash_func()         │
                    │          MD5+SHA1+SHA256 单次读取    │
                    └─────────────────────────────────────┘
```

核心设计要点：

- 任务只在入队/出队时持锁，**哈希计算在锁外执行**，降低锁竞争
- `while` 包裹 `cond_wait`，规避虚假唤醒
- 推任务用 `signal`（唤醒一个 worker），停机用 `broadcast`（唤醒全部）
- `terminate` 标志 + `pthread_join` 实现优雅停机与线程回收

## 目录结构

```
CHashProject/
├── CMakeLists.txt          # CMake 构建配置
├── include/
│   ├── threadpool.h        # 线程池 API 与结构定义
│   └── hash_utils.h        # 哈希任务与目录扫描声明
├── src/
│   ├── main.c              # 入口：参数解析、快速模式交互、计时统计
│   ├── threadpool.c        # 线程池实现（队列、同步、生命周期管理）
│   └── hash_utils.c        # 目录扫描、MD5/SHA1/SHA256 计算
└── test_data/              # 测试数据（小文本文件 + 嵌套目录）
```

> 测试用的 100MB 二进制文件 `big_file.bin` 未提交到仓库（超过 GitHub 单文件限制），可按需生成：
>
> ```bash
> dd if=/dev/urandom of=test_data/big_file.bin bs=1M count=100
> ```

## 快速开始

### 依赖

- GCC / Clang（支持 C99）
- CMake ≥ 3.10
- OpenSSL 开发库：`sudo apt install libssl-dev`
- pthread（Linux 自带）

### 构建

```bash
cd CHashProject
mkdir -p build && cd build
cmake ..
make
```

### 运行

```bash
# 基本用法：./c_hash_tool <目录>
./c_hash_tool ../test_data > report.csv
```

程序启动后会交互询问是否开启快速模式：

```
>>> Enable Fast Mode (calculate first 1MB only)? [y/N]: n
>>> [Status] Fast Mode DISABLED (Full Calculation).
>>> Scanning ../test_data ...
```

### 输出说明

**stdout → CSV 数据**（重定向到文件）：

```csv
FilePath,MD5,SHA1,SHA256
../test_data/text_1.txt,e72226f00b06a7b838d34f8ab943f1f9,41fb081873481344357af1d97f12eb8b6df8bbed,b70def233a18bd161fcdeee82a5e7d84b649cd1d631a32e335a605a2afea0dfc
...
--- Statistics ---
Metric,Value
Total Files,23
Total Time (s),0.101
Avg Time per File (ms),4.391
```

**stderr → 运行报告**（屏幕上直接看）：

```
================ REPORT ================
 Total Files   : 23
 Total Time    : 0.101 s
 Avg Time/File : 4.391 ms
========================================
```

> 由于多线程并行计算，CSV 中文件行的顺序不保证与目录遍历顺序一致。

## 性能数据

以下数据为快速模式（仅计算前 1MB）实测结果：

| 指标 | 数值 |
|---|---|
| 文件数 | 23（含 1 个 100MB 二进制文件） |
| 线程数 | 8 |
| 总耗时 | 0.101 s |
| 平均每文件 | 4.391 ms |

## 已知限制

- 线程数固定为 8，暂不支持命令行参数调整
- 目录扫描依赖 `d_type` 字段，部分文件系统（如 XFS）可能返回 `DT_UNKNOWN` 导致漏文件
- 暂未提供单线程模式，无法直接输出加速比对比
- 快速模式下哈希值仅基于文件前 1MB，不能用于完整性校验

## 许可证

MIT
