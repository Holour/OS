#include "memory/memory_manager.h"
#include <iostream>

MemoryManager::MemoryManager() : memory_pool(nullptr), used_memory(0) {}

MemoryManager::~MemoryManager() {
    delete[] memory_pool;
}

void MemoryManager::initialize() {
    // 实际分配内存
    memory_pool = new (std::nothrow) char[MEMORY_SIZE];
    if (memory_pool == nullptr) {
        throw std::runtime_error("Failed to allocate memory for the main memory pool.");
    }

    free_list.clear();
    free_list.push_back({0, MEMORY_SIZE});
    used_memory = 0;
    std::cout << "Memory manager initialized with " << MEMORY_SIZE / (1024*1024) << " MB of memory." << std::endl;
}

char* MemoryManager::get_memory_pool_ptr() {
    return memory_pool;
}

std::optional<uint64_t> MemoryManager::allocate(uint64_t size) {
    if (size == 0) {
        return std::nullopt;
    }

    // 首次适应算法
    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        if (it->size >= size) {
            uint64_t alloc_addr = it->base_address;
            it->base_address += size;
            it->size -= size;
            
            if (it->size == 0) {
                free_list.erase(it);
            }

            used_memory += size;
            return alloc_addr;
        }
    }

    // 内存不足
    return std::nullopt;
}

void MemoryManager::free(uint64_t base_address, uint64_t size) {
    if (size == 0) return;

    // 释放内存并尝试合并空闲块
    auto it = free_list.begin();
    while (it != free_list.end() && it->base_address < base_address) {
        ++it;
    }

    // 插入新的空闲块
    auto new_it = free_list.insert(it, {base_address, size});

    // 尝试与后一个块合并
    if (std::next(new_it) != free_list.end() && new_it->base_address + new_it->size == std::next(new_it)->base_address) {
        new_it->size += std::next(new_it)->size;
        free_list.erase(std::next(new_it));
    }
    
    // 尝试与前一个块合并
    if (new_it != free_list.begin() && std::prev(new_it)->base_address + std::prev(new_it)->size == new_it->base_address) {
        std::prev(new_it)->size += new_it->size;
        free_list.erase(new_it);
    }
    
    used_memory -= size;
}

const std::list<FreeBlock>& MemoryManager::get_free_blocks() const {
    return free_list;
}

uint64_t MemoryManager::get_total_memory() const {
    return MEMORY_SIZE;
}

uint64_t MemoryManager::get_used_memory() const {
    return used_memory;
} 