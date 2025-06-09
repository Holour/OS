#include "memory/memory_manager.h"
#include <iostream>
#include <cstring>

MemoryManager::MemoryManager() : memory_pool(nullptr), used_memory(0) {
    initialize();
}

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

std::optional<MemoryBlock> MemoryManager::allocate(uint64_t size) {
    if (size == 0) {
        return std::nullopt;
    }

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        if (it->size >= size) {
            uint64_t base_address = it->base_address;
            it->base_address += size;
            it->size -= size;
            if (it->size == 0) {
                free_list.erase(it);
            }
            used_memory += size;
            return MemoryBlock{base_address, size};
        }
    }

    return std::nullopt; // No suitable block found
}

bool MemoryManager::free(uint64_t base_address, uint64_t size) {
    if (size == 0) {
        return false;
    }
    
    used_memory -= size;

    // Add the new free block
    free_list.push_back({base_address, size});
    
    // Sort to bring adjacent blocks together
    free_list.sort([](const FreeBlock& a, const FreeBlock& b) {
        return a.base_address < b.base_address;
    });

    // Merge adjacent blocks
    auto it = free_list.begin();
    while (it != free_list.end()) {
        auto next_it = std::next(it);
        if (next_it == free_list.end()) {
            break; 
        }

        if (it->base_address + it->size == next_it->base_address) {
            it->size += next_it->size;
            free_list.erase(next_it);
            // After merging, stay at the current iterator 'it' to check if it can merge with the new 'next_it'
        } else {
            ++it; // Move to the next block if no merge occurred
        }
    }

    return true;
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

uint64_t MemoryManager::get_free_memory() const {
    uint64_t total_free = 0;
    for (const auto& block : free_list) {
        total_free += block.size;
    }
    return total_free;
}

std::string MemoryManager::read_memory(uint64_t address, size_t size) const {
    if (address + size > MEMORY_SIZE || memory_pool == nullptr) {
        throw std::runtime_error("Invalid memory access");
    }
    return std::string(memory_pool + address, size);
}

void MemoryManager::write_memory(uint64_t address, const std::string& data) {
    if (address + data.size() > MEMORY_SIZE || memory_pool == nullptr) {
        throw std::runtime_error("Invalid memory access");
    }
    std::copy(data.begin(), data.end(), memory_pool + address);
}

void MemoryManager::read_memory(uint64_t address, void* buffer, size_t size) const {
    if (address + size > MEMORY_SIZE || memory_pool == nullptr) {
        throw std::runtime_error("Invalid memory access in read");
    }
    std::memcpy(buffer, memory_pool + address, size);
}

void MemoryManager::write_memory(uint64_t address, const void* data, size_t size) {
    if (address + size > MEMORY_SIZE || memory_pool == nullptr) {
        throw std::runtime_error("Invalid memory access in write");
    }
    std::memcpy(memory_pool + address, data, size);
} 