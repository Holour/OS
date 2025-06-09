#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <ctime>
#include <map>
#include "../memory/memory_manager.h"

// Constants for the file system layout
const uint32_t BLOCK_SIZE = 4096;
const uint32_t NUM_BLOCKS = 1024; // 4MB disk size
const uint32_t MAX_INODES = 256;
const uint32_t DISK_SIZE_BYTES = NUM_BLOCKS * BLOCK_SIZE;
const int DIRECT_BLOCKS = 12;

enum class InodeType {
    FREE,
    FILE,
    DIRECTORY
};

struct Inode {
    InodeType type;
    uint32_t size;
    uint16_t ref_count;
    uint32_t direct_blocks[DIRECT_BLOCKS];
    uint16_t permissions;
    time_t created_at;
    time_t modified_at;
};

struct DirectoryEntry {
    char name[28];
    uint32_t inode_num;
};

struct FileContent {
    std::string path;
    std::string content;
    uint16_t permissions;
    uint32_t size;
    time_t created_at;
    time_t modified_at;
};

struct DirectoryContent {
    std::string name;
    std::string type;
    uint32_t size;
    uint16_t permissions;
    time_t created_at;
    time_t modified_at;
};

struct FileSystemStatus {
    uint64_t total_space;
    uint64_t used_space;
    uint64_t free_space;
    uint32_t total_files;
    uint32_t total_directories;
};

struct FileSystemLog {
    time_t timestamp;
    std::string operation;
    std::string path;
    std::string status;
    std::string details;
};

enum class FsDeleteResult {
    Success,
    NotFound,
    DirectoryNotEmpty,
    IsFile // Optional: if you want to be more specific
};

struct FileEntry {
    std::string name;
    bool is_directory;
    uint32_t size;
    uint16_t permissions;
};

class FileSystemManager {
public:
    explicit FileSystemManager(MemoryManager& mem_manager);
    ~FileSystemManager();

    void initialize();

    // Path-based operations
    bool create_directory(const std::string& path, uint16_t permissions);
    bool create_file(const std::string& path, const std::string& content, uint16_t permissions);
    std::optional<FileContent> read_file(const std::string& path);
    bool write_file(const std::string& path, const std::string& content, bool append = false);
    bool delete_file(const std::string& path);
    FsDeleteResult delete_directory(const std::string& path, bool recursive = false);
    std::optional<std::vector<DirectoryContent>> list_directory(const std::string& path);
    
    // Public helper for specific scenarios like error reporting in main.cpp
    std::optional<uint32_t> find_inode_by_path(const std::string& path, uint32_t* parent_inode_idx = nullptr, std::string* child_name = nullptr);

    // File system status
    FileSystemStatus get_filesystem_status();
    std::vector<FileSystemLog> get_filesystem_logs(const std::string& start_time = "", 
                                                 const std::string& end_time = "", 
                                                 const std::string& operation_type = "");

private:
    std::vector<DirectoryEntry> get_dir_entries(uint32_t inode_idx);
    bool add_dir_entry(uint32_t parent_inode_idx, uint32_t new_inode_idx, const std::string& name);
    bool remove_dir_entry(uint32_t parent_inode_idx, const std::string& name);

    std::optional<uint32_t> allocate_inode();
    void free_inode(uint32_t inode_num);
    std::optional<uint32_t> allocate_block();
    void free_block(uint32_t block_num);
    void log_operation(const std::string& operation, const std::string& path, 
                      const std::string& status, const std::string& details = "");

    MemoryManager& memory_manager;
    uint64_t disk_base_address;
    std::vector<Inode> inode_table;
    std::vector<bool> block_bitmap;
    std::vector<FileSystemLog> operation_logs;
};
