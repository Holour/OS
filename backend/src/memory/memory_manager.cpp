#include "memory/memory_manager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

MemoryManager::MemoryManager() : memory_pool(nullptr), used_memory(0), current_strategy(MemoryAllocationStrategy::CONTINUOUS) {
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

    // 初始化连续分配的空闲列表
    free_list.clear();
    free_list.push_back({0, MEMORY_SIZE});
    
    // 初始化分区
    initialize_partitions();
    
    // 初始化分页管理
    page_frames.reset(); // 所有页框初始为空闲
    page_tables.clear();
    
    used_memory = 0;
    std::cout << "Memory manager initialized with " << MEMORY_SIZE / (1024*1024) << " MB of memory." << std::endl;
}

void MemoryManager::set_allocation_strategy(MemoryAllocationStrategy strategy) {
    current_strategy = strategy;
    std::cout << "Memory allocation strategy changed to: " << static_cast<int>(strategy) << std::endl;
}

MemoryAllocationStrategy MemoryManager::get_allocation_strategy() const {
    return current_strategy;
}

void MemoryManager::initialize_partitions() {
    partitions.clear();
    
    // 创建不同大小的分区（固定分区算法）
    // 小分区: 256KB * 16 = 4MB
    for (int i = 0; i < 16; ++i) {
        partitions.emplace_back(i * 256 * 1024, 256 * 1024);
    }
    
    // 中分区: 1MB * 64 = 64MB  
    for (int i = 0; i < 64; ++i) {
        uint64_t base = 4 * 1024 * 1024 + i * 1024 * 1024;
        partitions.emplace_back(base, 1024 * 1024);
    }
    
    // 大分区: 4MB * 16 = 64MB
    for (int i = 0; i < 16; ++i) {
        uint64_t base = 68 * 1024 * 1024 + i * 4 * 1024 * 1024;
        partitions.emplace_back(base, 4 * 1024 * 1024);
    }
    
    // 超大分区: 32MB * 32 = 1024MB
    for (int i = 0; i < 32; ++i) {
        uint64_t base = 132 * 1024 * 1024 + i * 32 * 1024 * 1024;
        partitions.emplace_back(base, 32 * 1024 * 1024);
    }
    
    std::cout << "Initialized " << partitions.size() << " memory partitions." << std::endl;
}

char* MemoryManager::get_memory_pool_ptr() {
    return memory_pool;
}

std::optional<MemoryBlock> MemoryManager::allocate(uint64_t size) {
    return allocate_for_process(-1, size); // -1表示系统分配，不指定进程
}

std::optional<MemoryBlock> MemoryManager::allocate_for_process(ProcessID pid, uint64_t size) {
    if (size == 0) {
        return std::nullopt;
    }

    switch (current_strategy) {
        case MemoryAllocationStrategy::CONTINUOUS:
            return allocate_continuous(size);
            
        case MemoryAllocationStrategy::PARTITIONED:
            return allocate_partitioned(pid, size);
            
        case MemoryAllocationStrategy::PAGED:
            return allocate_paged(pid, size);
            
        default:
            return std::nullopt;
    }
}

std::optional<MemoryBlock> MemoryManager::allocate_continuous(uint64_t size) {
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
    return std::nullopt;
}

std::optional<MemoryBlock> MemoryManager::allocate_partitioned(ProcessID pid, uint64_t size) {
    // 找到合适大小的最小分区
    Partition* best_partition = nullptr;
    for (auto& partition : partitions) {
        if (partition.is_free && partition.size >= size) {
            if (best_partition == nullptr || partition.size < best_partition->size) {
                best_partition = &partition;
            }
        }
    }
    
    if (best_partition != nullptr) {
        best_partition->is_free = false;
        best_partition->owner_pid = pid;
        used_memory += best_partition->size;
        return MemoryBlock{best_partition->base_address, best_partition->size};
    }
    
    return std::nullopt;
}

std::optional<MemoryBlock> MemoryManager::allocate_paged(ProcessID pid, uint64_t size) {
    uint64_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE; // 向上取整
    
    if (pages_needed > (TOTAL_PAGES - page_frames.count())) {
        return std::nullopt; // 没有足够的页框
    }
    
    // 为进程创建页表（如果不存在）
    if (page_tables.find(pid) == page_tables.end()) {
        page_tables.emplace(pid, ProcessPageTable(pid));
    }
    
    auto& page_table = page_tables[pid];
    uint64_t virtual_base = page_table.pages.size() * PAGE_SIZE;
    
    // 分配所需的页框
    for (uint64_t i = 0; i < pages_needed; ++i) {
        uint64_t frame = allocate_free_frame();
        if (frame == UINT64_MAX) {
            // 分配失败，需要回滚已分配的页框
            for (uint64_t j = 0; j < i; ++j) {
                free_frame(page_table.pages[page_table.pages.size() - 1 - j].frame_number);
                page_table.pages.pop_back();
            }
            return std::nullopt;
        }
        
        PageTableEntry pte;
        pte.valid = true;
        pte.frame_number = frame;
        page_table.pages.push_back(pte);
    }
    
    used_memory += pages_needed * PAGE_SIZE;
    return MemoryBlock{virtual_base, pages_needed * PAGE_SIZE};
}

uint64_t MemoryManager::allocate_free_frame() {
    for (uint64_t i = 0; i < TOTAL_PAGES; ++i) {
        if (!page_frames[i]) {
            page_frames[i] = true;
            return i;
        }
    }
    return UINT64_MAX; // 没有空闲页框
}

void MemoryManager::free_frame(uint64_t frame_number) {
    if (frame_number < TOTAL_PAGES) {
        page_frames[frame_number] = false;
    }
}

bool MemoryManager::free(uint64_t base_address, uint64_t size) {
    if (current_strategy == MemoryAllocationStrategy::CONTINUOUS) {
        return free_continuous_memory(base_address, size);
    }
    
    // 对于分区和分页方式，需要更复杂的释放逻辑
    return false;
}

bool MemoryManager::free_continuous_memory(uint64_t base_address, uint64_t size) {
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
        } else {
            ++it;
        }
    }

    return true;
}

bool MemoryManager::free_process_memory(ProcessID pid) {
    switch (current_strategy) {
        case MemoryAllocationStrategy::PARTITIONED:
            return free_partitioned_memory(pid);
            
        case MemoryAllocationStrategy::PAGED:
            return free_pages_for_process(pid);
            
        default:
            return false; // 连续分配需要明确的地址和大小
    }
}

bool MemoryManager::free_partitioned_memory(ProcessID pid) {
    bool freed_any = false;
    for (auto& partition : partitions) {
        if (!partition.is_free && partition.owner_pid == pid) {
            partition.is_free = true;
            partition.owner_pid = -1;
            used_memory -= partition.size;
            freed_any = true;
        }
    }
    return freed_any;
}

bool MemoryManager::free_pages_for_process(ProcessID pid) {
    auto it = page_tables.find(pid);
    if (it == page_tables.end()) {
        return false;
    }
    
    auto& page_table = it->second;
    uint64_t freed_memory = 0;
    
    for (const auto& pte : page_table.pages) {
        if (pte.valid) {
            free_frame(pte.frame_number);
            freed_memory += PAGE_SIZE;
        }
    }
    
    page_tables.erase(it);
    used_memory -= freed_memory;
    return true;
}

uint64_t MemoryManager::get_process_base_address(ProcessID pid) const {
    switch (current_strategy) {
        case MemoryAllocationStrategy::PARTITIONED: {
            for (const auto& partition : partitions) {
                if (!partition.is_free && partition.owner_pid == pid) {
                    return partition.base_address;
                }
            }
            break;
        }
        
        case MemoryAllocationStrategy::PAGED: {
            // 分页方式下，进程看到的是虚拟地址，起始地址总是0
            auto it = page_tables.find(pid);
            if (it != page_tables.end()) {
                return 0; // 虚拟地址空间从0开始
            }
            break;
        }
        
        case MemoryAllocationStrategy::CONTINUOUS:
            // 连续分配需要从进程信息中获取，这里返回0作为默认值
            return 0;
    }
    
    return UINT64_MAX; // 未找到
}

std::optional<uint64_t> MemoryManager::translate_virtual_to_physical(ProcessID pid, uint64_t virtual_address) const {
    if (current_strategy != MemoryAllocationStrategy::PAGED) {
        return virtual_address; // 非分页模式，虚拟地址即物理地址
    }
    
    auto it = page_tables.find(pid);
    if (it == page_tables.end()) {
        return std::nullopt;
    }
    
    uint64_t page_number = virtual_address / PAGE_SIZE;
    uint64_t offset = virtual_address % PAGE_SIZE;
    
    const auto& page_table = it->second;
    if (page_number >= page_table.pages.size() || !page_table.pages[page_number].valid) {
        return std::nullopt;
    }
    
    uint64_t frame_number = page_table.pages[page_number].frame_number;
    return frame_number * PAGE_SIZE + offset;
}

const std::list<FreeBlock>& MemoryManager::get_free_blocks() const {
    return free_list;
}

std::vector<Partition> MemoryManager::get_partitions() const {
    return partitions;
}

uint64_t MemoryManager::get_total_memory() const {
    return MEMORY_SIZE;
}

uint64_t MemoryManager::get_used_memory() const {
    return used_memory;
}

uint64_t MemoryManager::get_free_memory() const {
    return MEMORY_SIZE - used_memory;
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