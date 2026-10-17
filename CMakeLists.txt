cmake_minimum_required(VERSION 3.10)
project(CHashCalculator C) # 明确指定是 C 项目

set(CMAKE_C_STANDARD 99) # 使用 C99 标准

include_directories(include)

# 查找库
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 扫描源文件
file(GLOB SOURCES "src/*.c")

add_executable(c_hash_tool ${SOURCES})

# 链接库
target_link_libraries(c_hash_tool OpenSSL::SSL OpenSSL::Crypto Threads::Threads)