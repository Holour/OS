#ifndef PCB_H
#define PCB_H

#include "../common.h"
#include <vector>
#include <cstdint>
#include <string>

// 内存块信息
struct MemoryBlock {
    uint64_t base_address;
    uint64_t size;

    // Default constructor
    MemoryBlock() : base_address(0), size(0) {}

    // Member-wise constructor
    MemoryBlock(uint64_t base, uint64_t sz) : base_address(base), size(sz) {}
};

// 进程控制块
class PCB {
public:
    ProcessID pid;
    ProcessState state;
    uint64_t program_counter;
    uint64_t cpu_time;           // 模拟 CPU 占用时间（总量）
    uint64_t remaining_time;     // 剩余 CPU 时间（调度用）
    uint32_t priority;           // 数字越小优先级越高
    uint64_t creation_time;      // 创建时间（毫秒 since epoch）
    std::string name;            // 进程名称
    ProcessID parent_pid;        // 父进程 ID（-1 表示无父进程 / 系统进程）
    // 可以添加寄存器等上下文信息
    // ...

    std::vector<MemoryBlock> memory_info; // 进程占用的内存块

    PCB()
        : pid(-1), state(ProcessState::NEW), program_counter(0), cpu_time(0), remaining_time(0), priority(0), creation_time(0),
          name(""), parent_pid(-1) {}
};

#endif //PCB_H 