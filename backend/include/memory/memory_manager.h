#pragma once

#include "../common.h"
#include <vector>
#include <cstdint>
#include <list>
#include <optional>
#include <map>
#include <bitset>
#include "../process/pcb.h" // For MemoryBlock

// 表示一个内存块
struct FreeBlock {
    uint64_t base_address;
    uint64_t size;
};

// 分区信息
struct Partition {
    uint64_t base_address;
    uint64_t size;
    bool is_free;
    ProcessID owner_pid;  // -1 表示空闲
    
    Partition() : base_address(0), size(0), is_free(true), owner_pid(-1) {}
    Partition(uint64_t base, uint64_t sz) : base_address(base), size(sz), is_free(true), owner_pid(-1) {}
};

// 页表项
struct PageTableEntry {
    bool valid;           // 页面是否有效
    uint64_t frame_number; // 物理页框号
    bool dirty;           // 脏位
    bool accessed;        // 访问位
    
    PageTableEntry() : valid(false), frame_number(0), dirty(false), accessed(false) {}
};

// 进程页表
struct ProcessPageTable {
    ProcessID pid;
    std::vector<PageTableEntry> pages;
    
    ProcessPageTable() : pid(-1) {}  // 默认构造函数
    ProcessPageTable(ProcessID p) : pid(p) {}
};

class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    // 初始化内存管理器
    void initialize();

    // 设置内存分配策略
    void set_allocation_strategy(MemoryAllocationStrategy strategy);
    MemoryAllocationStrategy get_allocation_strategy() const;

    // 分配指定大小的内存, 返回完整的内存块
    std::optional<MemoryBlock> allocate(uint64_t size);
    std::optional<MemoryBlock> allocate_for_process(ProcessID pid, uint64_t size);

    // 释放内存
    bool free(uint64_t base_address, uint64_t size);
    bool free_process_memory(ProcessID pid);

    // 获取内存使用情况 (for UI)
    const std::list<FreeBlock>& get_free_blocks() const;
    std::vector<Partition> get_partitions() const;
    uint64_t get_total_memory() const;
    uint64_t get_used_memory() const;
    uint64_t get_free_memory() const;
    char* get_memory_pool_ptr();

    // 内存读写操作（用于文件系统等）
    std::string read_memory(uint64_t address, size_t size) const;
    void read_memory(uint64_t address, void* buffer, size_t size) const;
    void write_memory(uint64_t address, const std::string& data);
    void write_memory(uint64_t address, const void* data, size_t size);

    // 获取进程起始地址
    uint64_t get_process_base_address(ProcessID pid) const;

    // 分页管理相关
    bool allocate_pages_for_process(ProcessID pid, uint64_t size);
    bool free_pages_for_process(ProcessID pid);
    std::optional<uint64_t> translate_virtual_to_physical(ProcessID pid, uint64_t virtual_address) const;

private:
    char* memory_pool; // 模拟内存池
    std::list<FreeBlock> free_list; // 空闲块链表 (连续分配使用)
    uint64_t used_memory;
    MemoryAllocationStrategy current_strategy;

    // 分区分配相关
    std::vector<Partition> partitions;
    void initialize_partitions();
    std::optional<MemoryBlock> allocate_partitioned(ProcessID pid, uint64_t size);

    // 分页分配相关
    std::bitset<TOTAL_PAGES> page_frames; // 页框使用位图
    std::map<ProcessID, ProcessPageTable> page_tables; // 进程页表
    std::optional<MemoryBlock> allocate_paged(ProcessID pid, uint64_t size);
    uint64_t allocate_free_frame();
    void free_frame(uint64_t frame_number);

    // 连续分配相关
    std::optional<MemoryBlock> allocate_continuous(uint64_t size);
    bool free_continuous_memory(uint64_t base_address, uint64_t size);
    
    // 分区分配辅助方法
    bool free_partitioned_memory(ProcessID pid);
}; 