#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <ctime>
#include <map>
#include <variant>

// Constants for the file system layout
const uint32_t BLOCK_SIZE = 4096; // 4 KB per block
const uint64_t DISK_SIZE_BYTES = 64ULL * 1024 * 1024 * 1024; // 64 GB disk size
const uint64_t NUM_BLOCKS = DISK_SIZE_BYTES / BLOCK_SIZE;
const uint32_t MAX_INODES = 1024; // Increased max inodes
const int DIRECT_BLOCKS = 10;
const uint32_t POINTERS_PER_BLOCK = BLOCK_SIZE / sizeof(uint32_t);

enum class InodeType {
    FREE,
    FILE,
    DIRECTORY
};

enum class AllocationStrategy {
    CONTIGUOUS,
    LINKED,
    INDEXED
};

// --- Block Pointer Structures for different allocation strategies ---
struct ContiguousAllocation {
    uint32_t start_block;
    uint32_t block_count;
};

struct LinkedAllocation {
    uint32_t start_block;
    uint32_t end_block; // To quickly append
};

struct IndexedAllocation {
    uint32_t index_block; // Block containing pointers to data blocks
};


struct Inode {
    InodeType type;
    uint64_t simulated_size; // << New field for simulated size
    uint16_t ref_count;
    
    // --- Flexible block info based on strategy ---
    std::variant<ContiguousAllocation, LinkedAllocation, IndexedAllocation> allocation_info;

    uint16_t permissions;
    time_t created_at;
    time_t modified_at;
};

struct DirectoryEntry {
    char name[52]; // Adjusted size
    uint32_t inode_num;
};

struct FileContent {
    std::string path;
    std::string content;
    uint16_t permissions;
    uint64_t simulated_size; // << Changed to uint64_t and represents simulated size
    time_t created_at;
    time_t modified_at;
};

struct DirectoryContent {
    std::string name;
    std::string type;
    uint64_t size; // Can be simulated_size for files
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
    AllocationStrategy allocation_method; // << New field
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
    uint64_t size; // Represents simulated size for files
    uint16_t permissions;
};

// Structure for returning file addresses
struct FileAddresses {
    std::optional<uint32_t> contiguous_start_block;
    std::optional<uint32_t> linked_start_block;
    std::optional<uint32_t> indexed_index_block;
};

enum class FsCreateResult {
    Success,
    AlreadyExists,
    ParentNotFound,
    InvalidPath
};

class FileSystemManager {
public:
    explicit FileSystemManager();
    ~FileSystemManager();

    void initialize();
    void set_allocation_strategy(AllocationStrategy strategy);

    // Path-based operations
    FsCreateResult create_directory(const std::string& path, uint16_t permissions);
    FsCreateResult create_file(const std::string& path, uint64_t simulated_size, uint16_t permissions); 
    std::optional<FileContent> read_file(const std::string& path);
    bool delete_file(const std::string& path);
    FsDeleteResult delete_directory(const std::string& path, bool recursive = false);
    std::optional<std::vector<DirectoryContent>> list_directory(const std::string& path);
    
    std::optional<uint32_t> find_inode_by_path(const std::string& path, uint32_t* parent_inode_idx = nullptr, std::string* child_name = nullptr);

    // File system status and info
    FileSystemStatus get_filesystem_status();
    FileAddresses get_file_addresses(const std::string& path);
    std::vector<FileSystemLog> get_filesystem_logs(const std::string& start_time = "", 
                                                 const std::string& end_time = "", 
                                                 const std::string& operation_type = "");

private:
    // --- Internal disk I/O helpers ---
    void read_disk(uint64_t offset, void* buffer, size_t size);
    void write_disk(uint64_t offset, const void* data, size_t size);

    std::vector<DirectoryEntry> get_dir_entries(uint32_t inode_idx);
    bool add_dir_entry(uint32_t parent_inode_idx, uint32_t new_inode_idx, const std::string& name);
    bool remove_dir_entry(uint32_t parent_inode_idx, const std::string& name);

    std::optional<uint32_t> allocate_inode();
    void free_inode(uint32_t inode_num);
    
    std::optional<uint32_t> allocate_block();
    std::optional<ContiguousAllocation> allocate_contiguous_blocks(uint32_t num_blocks);
    void free_block(uint32_t block_num);
    void free_blocks(const Inode& inode);

    void log_operation(const std::string& operation, const std::string& path, 
                      const std::string& status, const std::string& details = "");

    char* disk_pool; // << Manages its own disk space
    std::vector<Inode> inode_table;
    std::vector<bool> block_bitmap;
    std::vector<FileSystemLog> operation_logs;
    AllocationStrategy current_strategy;
};
