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

#endif // COMMON_H 