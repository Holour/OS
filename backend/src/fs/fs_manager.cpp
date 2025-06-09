#include "../../include/fs/fs_manager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath> // For std::ceil

// Helper to split a path into its components.
static std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    if (path.empty() || path == "/") return parts;
    std::stringstream ss(path);
    std::string part;
    if (ss.peek() == '/') ss.ignore();
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

// --- FileSystemManager Implementation ---

FileSystemManager::FileSystemManager()
    : disk_pool(nullptr),
      current_strategy(AllocationStrategy::INDEXED) {
    initialize();
}

FileSystemManager::~FileSystemManager() {
    delete[] disk_pool;
}

void FileSystemManager::initialize() {
    try {
        disk_pool = new char[DISK_SIZE_BYTES];
    } catch (const std::bad_alloc& e) {
        throw std::runtime_error("Failed to allocate memory for the file system disk.");
    }
    
    inode_table.resize(MAX_INODES);
    for (auto& inode : inode_table) {
        inode.type = InodeType::FREE;
    }

    block_bitmap.assign(NUM_BLOCKS, false);

    // Initialize root directory inode at index 0
    uint32_t root_inode_idx = 0;
    auto& root_inode = inode_table[root_inode_idx];
    root_inode.type = InodeType::DIRECTORY;
    root_inode.permissions = 0755;
    root_inode.ref_count = 1;
    root_inode.simulated_size = 0; // Directories use simulated_size to store entry count
    root_inode.created_at = std::time(nullptr);
    root_inode.modified_at = root_inode.created_at;

    // Root dir uses an indexed allocation style block to store its directory entries.
    auto block_num_opt = allocate_block();
    if (!block_num_opt) {
        throw std::runtime_error("Failed to allocate initial block for root directory.");
    }
    // For simplicity, we use IndexedAllocation for directories to hold their entries.
    root_inode.allocation_info = IndexedAllocation{*block_num_opt};
    
    log_operation("INIT", "/", "SUCCESS", "File system initialized");
}

void FileSystemManager::set_allocation_strategy(AllocationStrategy strategy) {
    this->current_strategy = strategy;
    log_operation("CONFIG", "SET_STRATEGY", "SUCCESS", "Set allocation strategy to " + std::to_string((int)strategy));
}

std::optional<uint32_t> FileSystemManager::find_inode_by_path(const std::string& path, uint32_t* parent_inode_idx, std::string* child_name) {
    if (path == "/") {
        if (parent_inode_idx) *parent_inode_idx = 0; // No real parent
        return 0;
    }

    auto parts = split_path(path);
    if (parts.empty()) return 0; // Root case again

    uint32_t current_inode_idx = 0; // Start from root
    uint32_t p_idx = 0;

    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& part = parts[i];
        if (inode_table[current_inode_idx].type != InodeType::DIRECTORY) {
            return std::nullopt;
        }

        auto entries = get_dir_entries(current_inode_idx);
        bool found = false;
        for (const auto& entry : entries) {
            if (entry.inode_num != 0 && strcmp(entry.name, part.c_str()) == 0) {
                p_idx = current_inode_idx;
                current_inode_idx = entry.inode_num;
                found = true;
                break;
            }
        }
        if (!found) {
            // If part not found, set parent info for creation and return nullopt
            if (i == parts.size() - 1) { // last part
                 if (parent_inode_idx) *parent_inode_idx = current_inode_idx;
                 if (child_name) *child_name = part;
            }
            return std::nullopt;
        }
    }
    
    if (parent_inode_idx) *parent_inode_idx = p_idx;
    if (child_name) *child_name = parts.back();

    return current_inode_idx;
}

FsCreateResult FileSystemManager::create_directory(const std::string& path, uint16_t permissions) {
    uint32_t parent_inode_idx;
    std::string new_dir_name;
    if (find_inode_by_path(path, &parent_inode_idx, &new_dir_name)) {
        log_operation("CREATE_DIR", path, "FAIL", "Path already exists");
        return FsCreateResult::AlreadyExists;
    }

    if (new_dir_name.empty()) {
        log_operation("CREATE_DIR", path, "FAIL", "Parent directory does not exist");
        return FsCreateResult::ParentNotFound;
    }

    auto new_inode_idx_opt = allocate_inode();
    if (!new_inode_idx_opt) {
        return FsCreateResult::InvalidPath; // Or some other internal error
    }
    
    // Directories need a data block to store their entries.
    auto block_num_opt = allocate_block();
    if (!block_num_opt) {
        free_inode(*new_inode_idx_opt);
        return FsCreateResult::InvalidPath;
    }

    auto& new_inode = inode_table[*new_inode_idx_opt];
    new_inode.type = InodeType::DIRECTORY;
    new_inode.simulated_size = 0; // Initially no entries
    new_inode.ref_count = 1;
    new_inode.permissions = permissions;
    new_inode.created_at = std::time(nullptr);
    new_inode.modified_at = std::time(nullptr);
    new_inode.allocation_info = IndexedAllocation{*block_num_opt}; // Store data block pointer

    if (!add_dir_entry(parent_inode_idx, *new_inode_idx_opt, new_dir_name)) {
        free_inode(*new_inode_idx_opt);
        free_block(*block_num_opt);
        return FsCreateResult::InvalidPath;
    }
    log_operation("CREATE_DIR", path, "SUCCESS");
    return FsCreateResult::Success;
}

FsCreateResult FileSystemManager::create_file(const std::string& path, uint64_t simulated_size, uint16_t permissions) {
    uint32_t parent_inode_idx;
    std::string filename;
    
    if (find_inode_by_path(path, &parent_inode_idx, &filename)) {
        log_operation("CREATE_FILE", path, "FAIL", "Path already exists");
        return FsCreateResult::AlreadyExists;
    }

    if (filename.empty()) {
        log_operation("CREATE_FILE", path, "FAIL", "Parent directory does not exist");
        return FsCreateResult::ParentNotFound;
    }

    auto new_inode_idx_opt = allocate_inode();
    if (!new_inode_idx_opt) {
        log_operation("CREATE_FILE", path, "FAIL", "Failed to allocate inode");
        return FsCreateResult::InvalidPath;
    }

    uint32_t num_blocks_needed = (simulated_size == 0) ? 0 : static_cast<uint32_t>(std::ceil(static_cast<double>(simulated_size) / BLOCK_SIZE));

    auto& new_inode = inode_table[*new_inode_idx_opt];
    new_inode.type = InodeType::FILE;
    new_inode.permissions = permissions;
    new_inode.simulated_size = simulated_size;
    new_inode.created_at = std::time(nullptr);
    new_inode.modified_at = new_inode.created_at;
    new_inode.ref_count = 1;

    bool allocation_success = false;

    switch (current_strategy) {
        case AllocationStrategy::CONTIGUOUS: {
            if (num_blocks_needed == 0) {
                new_inode.allocation_info = ContiguousAllocation{0, 0};
                allocation_success = true;
            } else {
                auto contiguous_opt = allocate_contiguous_blocks(num_blocks_needed);
                if (contiguous_opt) {
                    new_inode.allocation_info = *contiguous_opt;
                    allocation_success = true;
                }
            }
            break;
        }
        case AllocationStrategy::LINKED: {
            if (num_blocks_needed == 0) {
                new_inode.allocation_info = LinkedAllocation{0, 0};
                allocation_success = true;
            } else {
                std::vector<uint32_t> chain;
                for (uint32_t i = 0; i < num_blocks_needed; ++i) {
                    auto block_opt = allocate_block();
                    if (!block_opt) {
                        for (uint32_t block : chain) {
                            free_block(block);
                        }
                        free_inode(*new_inode_idx_opt);
                        log_operation("CREATE_FILE", path, "FAIL", "Failed to allocate blocks");
                        return FsCreateResult::InvalidPath;
                    }
                    chain.push_back(*block_opt);
                }
                
                for(size_t i = 0; i < chain.size() - 1; ++i) {
                    uint32_t next_block_num = chain[i+1];
                    uint64_t current_block_offset = (uint64_t)chain[i] * BLOCK_SIZE;
                    write_disk(current_block_offset + BLOCK_SIZE - sizeof(uint32_t), &next_block_num, sizeof(uint32_t));
                }
                new_inode.allocation_info = LinkedAllocation{chain.front(), chain.back()};
                allocation_success = true;
            }
            break;
        }
        case AllocationStrategy::INDEXED: {
            auto index_block_opt = allocate_block();
            if (!index_block_opt) {
                free_inode(*new_inode_idx_opt);
                log_operation("CREATE_FILE", path, "FAIL", "Failed to allocate index block");
                return FsCreateResult::InvalidPath;
            }
            
            if (num_blocks_needed == 0) {
                new_inode.allocation_info = IndexedAllocation{*index_block_opt};
                allocation_success = true;
            } else {
                std::vector<uint32_t> data_blocks;
                for (uint32_t i = 0; i < num_blocks_needed; ++i) {
                    auto block_opt = allocate_block();
                    if (!block_opt) {
                        for (uint32_t block : data_blocks) {
                            free_block(block);
                        }
                        free_block(*index_block_opt);
                        free_inode(*new_inode_idx_opt);
                        log_operation("CREATE_FILE", path, "FAIL", "Failed to allocate data blocks");
                        return FsCreateResult::InvalidPath;
                    }
                    data_blocks.push_back(*block_opt);
                }
                
                uint64_t index_block_offset = (uint64_t)(*index_block_opt) * BLOCK_SIZE;
                write_disk(index_block_offset, data_blocks.data(), data_blocks.size() * sizeof(uint32_t));
                
                new_inode.allocation_info = IndexedAllocation{*index_block_opt};
                allocation_success = true;
            }
            break;
        }
    }

    if (!allocation_success) {
        free_inode(*new_inode_idx_opt);
        log_operation("CREATE_FILE", path, "FAIL", "Block allocation failed");
        return FsCreateResult::InvalidPath;
    }

    if (!add_dir_entry(parent_inode_idx, *new_inode_idx_opt, filename)) {
        free_blocks(new_inode);
        free_inode(*new_inode_idx_opt);
        log_operation("CREATE_FILE", path, "FAIL", "Failed to add directory entry");
        return FsCreateResult::InvalidPath;
    }

    log_operation("CREATE_FILE", path, "SUCCESS");
    return FsCreateResult::Success;
}

std::optional<FileContent> FileSystemManager::read_file(const std::string& path) {
    auto inode_idx_opt = find_inode_by_path(path);
    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::FILE) return std::nullopt;
    
    const auto& inode = inode_table[*inode_idx_opt];
    
    // For this simulation, we don't read the actual (potentially massive) data.
    // We just return metadata and a placeholder content.
    std::string placeholder_content = "Simulated file content for: " + path + "\n";
    placeholder_content += "Simulated Size: " + std::to_string(inode.simulated_size) + " bytes\n";

    return FileContent{path, placeholder_content, inode.permissions, inode.simulated_size, inode.created_at, inode.modified_at};
}

bool FileSystemManager::delete_file(const std::string& path) {
    uint32_t parent_inode_idx;
    std::string filename;
    auto inode_idx_opt = find_inode_by_path(path, &parent_inode_idx, &filename);

    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::FILE) {
        log_operation("DELETE_FILE", path, "FAIL", "File not found or not a file");
        return false;
    }
    
    const auto& inode_to_delete = inode_table[*inode_idx_opt];
    
    // Free all associated blocks
    free_blocks(inode_to_delete);

    // Remove directory entry
    if (!remove_dir_entry(parent_inode_idx, filename)) {
        // This case is unlikely if find_inode_by_path succeeded, but handle it.
        log_operation("DELETE_FILE", path, "FAIL", "Failed to remove directory entry");
        // We don't re-allocate blocks, as the file is in an inconsistent state.
        return false;
    }

    // Free the inode itself
    free_inode(*inode_idx_opt);

    log_operation("DELETE_FILE", path, "SUCCESS");
    return true;
}

FsDeleteResult FileSystemManager::delete_directory(const std::string& path, bool recursive) {
    if (path == "/") return FsDeleteResult::DirectoryNotEmpty; // Cannot delete root
    
    uint32_t parent_idx;
    std::string child_name;
    auto inode_idx_opt = find_inode_by_path(path, &parent_idx, &child_name);

    if (!inode_idx_opt) return FsDeleteResult::NotFound;

    auto& inode = inode_table[*inode_idx_opt];
    if (inode.type != InodeType::DIRECTORY) return FsDeleteResult::IsFile;

    auto entries = get_dir_entries(*inode_idx_opt);
    if (!entries.empty() && !recursive) return FsDeleteResult::DirectoryNotEmpty;

    for (const auto& entry : entries) {
        std::string full_path = path + "/" + entry.name;
        if (inode_table[entry.inode_num].type == InodeType::DIRECTORY) {
            delete_directory(full_path, true);
        } else {
            delete_file(full_path);
        }
    }

    // Free the directory's own data block
    free_blocks(inode);

    remove_dir_entry(parent_idx, child_name);
    free_inode(*inode_idx_opt);

    log_operation("DELETE_DIR", path, "SUCCESS");
    return FsDeleteResult::Success;
}

std::optional<std::vector<DirectoryContent>> FileSystemManager::list_directory(const std::string& path) {
    auto inode_idx_opt = find_inode_by_path(path);
    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::DIRECTORY) {
        return std::nullopt;
    }

    auto entries = get_dir_entries(*inode_idx_opt);
    std::vector<DirectoryContent> contents;

    for (const auto& entry : entries) {
        if (entry.inode_num != 0) {
            const auto& inode = inode_table[entry.inode_num];
            DirectoryContent dc;
            dc.name = entry.name;
            dc.type = (inode.type == InodeType::DIRECTORY) ? "directory" : "file";
            dc.size = inode.simulated_size;
            dc.permissions = inode.permissions;
            dc.created_at = inode.created_at;
            dc.modified_at = inode.modified_at;
            contents.push_back(dc);
        }
    }
    return contents;
}

std::vector<DirectoryEntry> FileSystemManager::get_dir_entries(uint32_t inode_idx) {
    std::vector<DirectoryEntry> entries;
    const auto& inode = inode_table[inode_idx];
    if (inode.type != InodeType::DIRECTORY || inode.simulated_size == 0) {
        return entries;
    }

    // Directories use an index block to hold their entries
    auto* alloc_info = std::get_if<IndexedAllocation>(&inode.allocation_info);
    if (!alloc_info) return entries; // Should not happen for directories

    uint64_t data_block_offset = (uint64_t)alloc_info->index_block * BLOCK_SIZE;
    uint32_t num_entries = inode.simulated_size;
    
    std::vector<char> buffer(num_entries * sizeof(DirectoryEntry));
    read_disk(data_block_offset, buffer.data(), buffer.size());

    for (uint32_t i = 0; i < num_entries; ++i) {
        DirectoryEntry entry;
        std::memcpy(&entry, buffer.data() + i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
        entries.push_back(entry);
    }
    
    return entries;
}

bool FileSystemManager::add_dir_entry(uint32_t parent_inode_idx, uint32_t new_inode_idx, const std::string& name) {
    auto& parent_inode = inode_table[parent_inode_idx];
    if (parent_inode.type != InodeType::DIRECTORY) return false;

    auto entries = get_dir_entries(parent_inode_idx);
    
    if (entries.size() * sizeof(DirectoryEntry) + sizeof(DirectoryEntry) > BLOCK_SIZE) {
        return false; // Directory is full
    }

    DirectoryEntry new_entry;
    new_entry.inode_num = new_inode_idx;
    strncpy(new_entry.name, name.c_str(), sizeof(new_entry.name) - 1);
    new_entry.name[sizeof(new_entry.name) - 1] = '\0';
    entries.push_back(new_entry);

    auto* alloc_info = std::get_if<IndexedAllocation>(&parent_inode.allocation_info);
    if (!alloc_info) return false;

    uint64_t data_block_offset = (uint64_t)alloc_info->index_block * BLOCK_SIZE;
    write_disk(data_block_offset, entries.data(), entries.size() * sizeof(DirectoryEntry));
    
    parent_inode.simulated_size = entries.size(); // Update entry count
    parent_inode.modified_at = std::time(nullptr);
    
    return true;
}

bool FileSystemManager::remove_dir_entry(uint32_t parent_inode_idx, const std::string& name) {
    auto& parent_inode = inode_table[parent_inode_idx];
    if (parent_inode.type != InodeType::DIRECTORY) return false;

    auto entries = get_dir_entries(parent_inode_idx);
    auto it = std::remove_if(entries.begin(), entries.end(), [&](const DirectoryEntry& entry){
        return name == entry.name;
    });

    if (it == entries.end()) {
        return false; // Entry not found
    }
    
    entries.erase(it, entries.end());

    auto* alloc_info = std::get_if<IndexedAllocation>(&parent_inode.allocation_info);
    if (!alloc_info) return false;

    uint64_t data_block_offset = (uint64_t)alloc_info->index_block * BLOCK_SIZE;
    write_disk(data_block_offset, entries.data(), entries.size() * sizeof(DirectoryEntry));

    parent_inode.simulated_size = entries.size();
    parent_inode.modified_at = std::time(nullptr);

    return true;
}

void FileSystemManager::log_operation(const std::string& operation, const std::string& path, 
                      const std::string& status, const std::string& details) {
    operation_logs.push_back({std::time(nullptr), operation, path, status, details});
}

std::optional<uint32_t> FileSystemManager::allocate_inode() {
    for (uint32_t i = 1; i < MAX_INODES; ++i) {
        if (inode_table[i].type == InodeType::FREE) {
            inode_table[i].ref_count = 1;
            return i;
        }
    }
    return std::nullopt;
}

void FileSystemManager::free_inode(uint32_t inode_num) {
    if (inode_num < MAX_INODES) {
        inode_table[inode_num] = {};
        inode_table[inode_num].type = InodeType::FREE;
    }
}

std::optional<uint32_t> FileSystemManager::allocate_block() {
    for (uint32_t i = 0; i < NUM_BLOCKS; ++i) {
        if (!block_bitmap[i]) {
            block_bitmap[i] = true;
            return i;
        }
    }
    return std::nullopt;
}

void FileSystemManager::free_block(uint32_t block_num) {
    if (block_num < NUM_BLOCKS) {
        block_bitmap[block_num] = false;
    }
}

FileSystemStatus FileSystemManager::get_filesystem_status() {
    uint64_t used_blocks = 0;
    for(bool used : block_bitmap) {
        if(used) used_blocks++;
    }

    uint32_t files = 0;
    uint32_t dirs = 0;
    for(const auto& inode : inode_table) {
        if(inode.type == InodeType::FILE) files++;
        else if (inode.type == InodeType::DIRECTORY) dirs++;
    }
    
    uint64_t used_space = used_blocks * BLOCK_SIZE;

    return {
        DISK_SIZE_BYTES,
        used_space,
        DISK_SIZE_BYTES - used_space,
        files,
        dirs,
        current_strategy
    };
}

std::vector<FileSystemLog> FileSystemManager::get_filesystem_logs(const std::string& start_time, const std::string& end_time, const std::string& operation_type) {
    // Implementation for log filtering if needed
    return operation_logs;
}

std::optional<ContiguousAllocation> FileSystemManager::allocate_contiguous_blocks(uint32_t num_blocks) {
    if (num_blocks == 0) return ContiguousAllocation{0, 0};

    uint32_t consecutive_found = 0;
    uint32_t start_block = 0;

    for (uint64_t i = 0; i < NUM_BLOCKS; ++i) {
        if (!block_bitmap[i]) {
            if (consecutive_found == 0) {
                start_block = i;
            }
            consecutive_found++;
            if (consecutive_found == num_blocks) {
                // Found a spot, mark as used
                for (uint32_t j = 0; j < num_blocks; ++j) {
                    block_bitmap[start_block + j] = true;
                }
                return ContiguousAllocation{start_block, num_blocks};
            }
        } else {
            consecutive_found = 0;
        }
    }
    return std::nullopt; // No contiguous space found
}

void FileSystemManager::free_blocks(const Inode& inode) {
    std::visit([this, &inode](auto&& arg) { // Capture inode by reference
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ContiguousAllocation>) {
            for (uint32_t i = 0; i < arg.block_count; ++i) {
                free_block(arg.start_block + i);
            }
        } else if constexpr (std::is_same_v<T, LinkedAllocation>) {
            uint32_t current_block = arg.start_block;
            if (current_block == 0) return;
            while(current_block != arg.end_block) {
                uint64_t block_offset = (uint64_t)current_block * BLOCK_SIZE;
                uint32_t next_block = 0;
                read_disk(block_offset + BLOCK_SIZE - sizeof(uint32_t), &next_block, sizeof(uint32_t));
                free_block(current_block);
                current_block = next_block;
            }
            if (current_block == arg.end_block) {
                free_block(arg.end_block);
            }
        } else if constexpr (std::is_same_v<T, IndexedAllocation>) {
            // For directories, this just frees the entry block.
            // For files, it frees the index block and all data blocks.
            if (inode.type == InodeType::FILE) {
                uint32_t num_pointers = (inode.simulated_size == 0) ? 0 : static_cast<uint32_t>(std::ceil(static_cast<double>(inode.simulated_size) / BLOCK_SIZE));
                if (num_pointers > 0) {
                    std::vector<uint32_t> data_blocks(num_pointers);
                    uint64_t index_block_offset = (uint64_t)arg.index_block * BLOCK_SIZE;
                    read_disk(index_block_offset, data_blocks.data(), num_pointers * sizeof(uint32_t));
                    for(uint32_t block_num : data_blocks) {
                        if (block_num != 0) free_block(block_num);
                    }
                }
            }
            free_block(arg.index_block);
        }
    }, inode.allocation_info);
}

FileAddresses FileSystemManager::get_file_addresses(const std::string& path) {
    auto inode_idx_opt = find_inode_by_path(path);
    if (!inode_idx_opt) return {};

    const auto& inode = inode_table[*inode_idx_opt];
    if (inode.type != InodeType::FILE) return {};

    FileAddresses addresses{};
    
    std::visit([&addresses](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ContiguousAllocation>) {
            addresses.contiguous_start_block = arg.start_block;
        } else if constexpr (std::is_same_v<T, LinkedAllocation>) {
            addresses.linked_start_block = arg.start_block;
        } else if constexpr (std::is_same_v<T, IndexedAllocation>) {
            addresses.indexed_index_block = arg.index_block;
        }
    }, inode.allocation_info);

    return addresses;
}

void FileSystemManager::read_disk(uint64_t offset, void* buffer, size_t size) {
    if (offset + size > DISK_SIZE_BYTES) {
        throw std::runtime_error("Disk read out of bounds.");
    }
    std::memcpy(buffer, disk_pool + offset, size);
}

void FileSystemManager::write_disk(uint64_t offset, const void* data, size_t size) {
    if (offset + size > DISK_SIZE_BYTES) {
        throw std::runtime_error("Disk write out of bounds.");
    }
    std::memcpy(disk_pool + offset, data, size);
}