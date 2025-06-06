#include "fs/fs_manager.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring> // For memcpy and strncpy

// Helper to split a string by a delimiter
std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> components;
    if (path == "/") {
        return components;
    }
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            components.push_back(item);
        }
    }
    return components;
}

// Constructor: Allocate a contiguous block of memory for the "disk"
FileSystemManager::FileSystemManager(MemoryManager& mm) 
    : overall_memory_manager(mm), 
      inode_table(MAX_INODES), 
      block_bitmap(NUM_BLOCKS, false),
      disk_base_address(0) 
{
    auto disk_memory_opt = overall_memory_manager.allocate(SIM_DISK_SIZE);
    if (!disk_memory_opt) {
        throw std::runtime_error("Failed to allocate memory for the virtual disk.");
    }
    // We get the base pointer of the whole memory pool, and use our allocated address as an offset.
    char* memory_pool_base = overall_memory_manager.get_memory_pool_ptr();
    disk_base_address = disk_memory_opt.value();

    std::cout << "FileSystemManager initialized, allocated " << SIM_DISK_SIZE / (1024*1024) << "MB for disk." << std::endl;
}

// Format the file system
void FileSystemManager::format() {
    for (auto& inode : inode_table) {
        inode.mode = InodeMode::UNALLOCATED;
    }
    std::fill(block_bitmap.begin(), block_bitmap.end(), false);

    auto root_inode_num_opt = allocate_inode();
    if (!root_inode_num_opt || root_inode_num_opt.value() != 0) {
        throw std::runtime_error("Failed to allocate root inode during format.");
    }
    
    inode_table[0].mode = InodeMode::DIRECTORY;
    inode_table[0].size = 0;
    inode_table[0].block_count = 0;
    inode_table[0].last_modified_time = time(nullptr);
    
    // In a real FS, the root dir would have '.' and '..' but we simplify for now.
    // The entries will be added when creating subdirectories.
    
    std::cout << "File system formatted. Root directory created at inode 0." << std::endl;
}

// --- Inode and Block Management ---
std::optional<uint32_t> FileSystemManager::allocate_inode() {
    for (uint32_t i = 0; i < inode_table.size(); ++i) {
        if (inode_table[i].mode == InodeMode::UNALLOCATED) {
            inode_table[i].mode = InodeMode::FILE; // Mark as allocated to prevent re-allocation
            return i;
        }
    }
    return std::nullopt;
}

std::optional<uint32_t> FileSystemManager::allocate_block() {
    for (uint32_t i = 0; i < block_bitmap.size(); ++i) {
        if (!block_bitmap[i]) {
            block_bitmap[i] = true;
            return i;
        }
    }
    return std::nullopt;
}

void FileSystemManager::free_block(uint32_t block_num) {
    if (block_num < block_bitmap.size()) {
        block_bitmap[block_num] = false;
    }
}

std::optional<Inode> FileSystemManager::get_inode(uint32_t inode_number) const {
    if (inode_number >= inode_table.size()) return std::nullopt;
    return inode_table[inode_number];
}


// --- Path and Directory Management ---
std::optional<uint32_t> FileSystemManager::find_inode_for_path(const std::string& path) {
    if (path == "/") return 0;
    
    auto components = split_path(path);
    uint32_t current_inode_num = 0;

    for (const auto& component : components) {
        auto inode_opt = get_inode(current_inode_num);
        if (!inode_opt || inode_opt->mode != InodeMode::DIRECTORY) return std::nullopt;

        bool found_next = false;
        for (int i = 0; i < DIRECT_BLOCKS && inode_opt->direct_blocks[i] != 0; ++i) {
            uint32_t block_num = inode_opt->direct_blocks[i];
            char* block_ptr = overall_memory_manager.get_memory_pool_ptr() + disk_base_address + block_num * BLOCK_SIZE;
            DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(block_ptr);
            
            for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); ++j) {
                // Inode 0 is not valid for a directory entry, use it to mark empty.
                if (entries[j].inode_number != 0 && strcmp(entries[j].name, component.c_str()) == 0) {
                    current_inode_num = entries[j].inode_number;
                    found_next = true;
                    break;
                }
            }
            if (found_next) break;
        }
        if (!found_next) return std::nullopt;
    }
    return current_inode_num;
}

bool FileSystemManager::add_directory_entry(uint32_t parent_inode_num, uint32_t new_inode_num, const std::string& new_name) {
    // Note: This is a reference, so changes to parent_inode will be reflected in inode_table.
    Inode& parent_inode = inode_table[parent_inode_num];
    
    // Find an empty slot in existing data blocks
    for(int i = 0; i < DIRECT_BLOCKS; ++i) {
        if (parent_inode.direct_blocks[i] == 0) { // No block allocated here yet, need to allocate one.
             auto new_block_opt = allocate_block();
             if (!new_block_opt) return false; // No space on disk
             parent_inode.direct_blocks[i] = new_block_opt.value();
             parent_inode.block_count++;
             // Zero out the new block
             char* block_ptr = overall_memory_manager.get_memory_pool_ptr() + disk_base_address + parent_inode.direct_blocks[i] * BLOCK_SIZE;
             memset(block_ptr, 0, BLOCK_SIZE);
        }
        
        char* block_ptr = overall_memory_manager.get_memory_pool_ptr() + disk_base_address + parent_inode.direct_blocks[i] * BLOCK_SIZE;
        DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(block_ptr);
        for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); ++j) {
            if (entries[j].inode_number == 0) { // inode 0 marks an empty entry
                entries[j].inode_number = new_inode_num;
                strncpy(entries[j].name, new_name.c_str(), sizeof(entries[j].name) - 1);
                entries[j].name[sizeof(entries[j].name) - 1] = '\0';
                parent_inode.size += sizeof(DirectoryEntry);
                parent_inode.last_modified_time = time(nullptr);
                return true;
            }
        }
    }

    return false; // Directory is full
}

std::optional<uint32_t> FileSystemManager::create_directory(const std::string& path, const std::string& parent_path) {
    auto parent_inode_num_opt = find_inode_for_path(parent_path);
    if (!parent_inode_num_opt) return std::nullopt;
    uint32_t parent_inode_num = parent_inode_num_opt.value();
    
    std::string new_dir_name = path.substr(parent_path == "/" ? 1 : parent_path.length() + 1);

    auto new_inode_num_opt = allocate_inode();
    if (!new_inode_num_opt) return std::nullopt;
    uint32_t new_inode_num = new_inode_num_opt.value();

    auto new_block_num_opt = allocate_block();
    if (!new_block_num_opt) {
        inode_table[new_inode_num].mode = InodeMode::UNALLOCATED;
        return std::nullopt;
    }
    uint32_t new_block_num = new_block_num_opt.value();

    // Setup the new inode
    inode_table[new_inode_num].mode = InodeMode::DIRECTORY;
    inode_table[new_inode_num].size = sizeof(DirectoryEntry) * 2;
    inode_table[new_inode_num].block_count = 1;
    inode_table[new_inode_num].direct_blocks[0] = new_block_num;
    inode_table[new_inode_num].last_modified_time = time(nullptr);

    char* block_ptr = overall_memory_manager.get_memory_pool_ptr() + disk_base_address + new_block_num * BLOCK_SIZE;
    DirectoryEntry* new_block_entries = reinterpret_cast<DirectoryEntry*>(block_ptr);
    memset(new_block_entries, 0, BLOCK_SIZE); 

    // '.' entry
    new_block_entries[0].inode_number = new_inode_num;
    strncpy(new_block_entries[0].name, ".", sizeof(new_block_entries[0].name));
    
    // '..' entry
    new_block_entries[1].inode_number = parent_inode_num;
    strncpy(new_block_entries[1].name, "..", sizeof(new_block_entries[1].name));

    if (!add_directory_entry(parent_inode_num, new_inode_num, new_dir_name)) {
        inode_table[new_inode_num].mode = InodeMode::UNALLOCATED;
        free_block(new_block_num);
        return std::nullopt;
    }
    
    return new_inode_num;
}

std::optional<uint32_t> FileSystemManager::create_file(const std::string& path, const std::string& parent_path) {
    // TODO: Stub for now
    return std::nullopt;
}

// --- Implementation for Testing ---
bool FileSystemManager::read_block_for_test(uint32_t block_num, char* buffer, uint32_t buffer_size) {
    if (block_num >= NUM_BLOCKS || buffer_size < BLOCK_SIZE) {
        return false;
    }
    char* block_ptr = overall_memory_manager.get_memory_pool_ptr() + disk_base_address + block_num * BLOCK_SIZE;
    memcpy(buffer, block_ptr, BLOCK_SIZE);
    return true;
} 