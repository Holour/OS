#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include "inode.h"
#include "../memory/memory_manager.h"
#include <vector>
#include <string>
#include <optional>

const int MAX_INODES = 1024;
// 模拟磁盘大小 (e.g., 10MB)
const uint64_t SIM_DISK_SIZE = 10 * 1024 * 1024; 
const uint32_t NUM_BLOCKS = SIM_DISK_SIZE / BLOCK_SIZE;

class FileSystemManager {
public:
    FileSystemManager(MemoryManager& mm);
    void format();
    std::optional<uint32_t> create_file(const std::string& path, const std::string& parent_path);
    std::optional<uint32_t> create_directory(const std::string& path, const std::string& parent_path);
    
    std::optional<Inode> get_inode(uint32_t inode_number) const;
    
    // --- Public for Testing ---
    bool read_block_for_test(uint32_t block_num, char* buffer, uint32_t buffer_size);
    
private:
    MemoryManager& overall_memory_manager; // The OS's main memory manager
    
    // Disk simulation
    uint64_t disk_base_address; // Starting address of our "disk" in main memory
    std::vector<bool> block_bitmap; // To track free/used data blocks

    std::vector<Inode> inode_table;

    // --- Private Helper Functions ---
    // Inode management
    std::optional<uint32_t> allocate_inode();
    
    // Block management
    std::optional<uint32_t> allocate_block();
    void free_block(uint32_t block_num);
    
    // Path and directory management
    std::optional<uint32_t> find_inode_for_path(const std::string& path);
    bool add_directory_entry(uint32_t parent_inode_num, uint32_t new_inode_num, const std::string& new_name);
};

#endif //FS_MANAGER_H
