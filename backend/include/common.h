#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <string>
#include <optional>

// 模拟 4GB 内存
const uint64_t MEMORY_SIZE = 4ULL * 1024 * 1024 * 1024;

// Process ID type
using ProcessID = int32_t;

// Memory address type
using MemoryAddress = uint64_t;

// Process states
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// Interrupt types
enum class InterruptType {
    TIMER,
    DEVICE_IO,
    SYSTEM_CALL
};

// 内存分配策略
enum class MemoryAllocationStrategy {
    CONTINUOUS,     // 连续分配
    PARTITIONED,    // 分区分配
    PAGED          // 分页分配
};

// 分页相关常量
const uint64_t PAGE_SIZE = 4096;  // 4KB页面大小
const uint64_t TOTAL_PAGES = MEMORY_SIZE / PAGE_SIZE;

#endif // COMMON_H 