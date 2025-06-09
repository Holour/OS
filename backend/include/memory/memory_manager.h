#pragma once

#include "../common.h"
#include <vector>
#include <cstdint>
#include <list>
#include <optional>
#include "../process/pcb.h" // For MemoryBlock

// 表示一个内存块
struct FreeBlock {
    uint64_t base_address;
    uint64_t size;
};

class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    // 初始化内存管理器
    void initialize();

    // 分配指定大小的内存, 返回完整的内存块
    std::optional<MemoryBlock> allocate(uint64_t size);

    // 释放内存
    bool free(uint64_t base_address, uint64_t size);

    // 获取内存使用情况 (for UI)
    const std::list<FreeBlock>& get_free_blocks() const;
    uint64_t get_total_memory() const;
    uint64_t get_used_memory() const;
    uint64_t get_free_memory() const;
    char* get_memory_pool_ptr();

    // 内存读写操作（用于文件系统等）
    std::string read_memory(uint64_t address, size_t size) const;
    void write_memory(uint64_t address, const std::string& data);

private:
    char* memory_pool; // 模拟内存池
    std::list<FreeBlock> free_list; // 空闲块链表
    uint64_t used_memory;
}; 