#ifndef PCB_H
#define PCB_H

#include "../common.h"
#include <vector>
#include <cstdint>

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
    // 可以添加寄存器等上下文信息
    // ...

    std::vector<MemoryBlock> memory_info; // 进程占用的内存块

    PCB() : pid(-1), state(ProcessState::NEW) {}
};

#endif //PCB_H 