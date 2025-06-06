#ifndef PCB_H
#define PCB_H

#include "../common.h"
#include <vector>
#include <cstdint>

// 进程状态
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// 内存块信息
struct MemoryBlock {
    uint64_t base_address;
    uint64_t size;
};

// 进程控制块
class PCB {
public:
    pid_t pid;
    ProcessState state;
    uint64_t program_counter;
    // 可以添加寄存器等上下文信息
    // ...

    std::vector<MemoryBlock> memory_info; // 进程占用的内存块

    PCB(pid_t id);
};

#endif //PCB_H 