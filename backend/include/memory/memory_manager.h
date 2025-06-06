#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "../common.h"
#include <vector>
#include <cstdint>
#include <list>
#include <optional>

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

    // 分配指定大小的内存, 返回基地址
    std::optional<uint64_t> allocate(uint64_t size);

    // 释放内存
    void free(uint64_t base_address, uint64_t size);

    // 获取内存使用情况 (for UI)
    const std::list<FreeBlock>& get_free_blocks() const;
    uint64_t get_total_memory() const;
    uint64_t get_used_memory() const;
    char* get_memory_pool_ptr();


private:
    char* memory_pool; // 模拟内存池
    std::list<FreeBlock> free_list; // 空闲块链表
    uint64_t used_memory;
};

#endif //MEMORY_MANAGER_H 