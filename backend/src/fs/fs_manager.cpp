#include "../../include/fs/fs_manager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <vector>

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

FileSystemManager::FileSystemManager(MemoryManager& mem_manager)
    : memory_manager(mem_manager), disk_base_address(0) {
    initialize();
}

FileSystemManager::~FileSystemManager() {}

void FileSystemManager::initialize() {
    auto disk_memory_block = memory_manager.allocate(DISK_SIZE_BYTES);
    if (!disk_memory_block) {
        throw std::runtime_error("Failed to allocate memory for the file system disk.");
    }
    disk_base_address = disk_memory_block->base_address;

    inode_table.resize(MAX_INODES);
    for (auto& inode : inode_table) {
        inode.type = InodeType::FREE;
    }

    block_bitmap.resize(NUM_BLOCKS, false);

    // Initialize root directory inode at index 0
    uint32_t root_inode_idx = 0;
    inode_table[root_inode_idx].type = InodeType::DIRECTORY;
    inode_table[root_inode_idx].permissions = 0755;
    inode_table[root_inode_idx].ref_count = 1;
    inode_table[root_inode_idx].size = 0;
    inode_table[root_inode_idx].created_at = std::time(nullptr);
    inode_table[root_inode_idx].modified_at = inode_table[root_inode_idx].created_at;

    auto block_num_opt = allocate_block();
    if (!block_num_opt) {
        throw std::runtime_error("Failed to allocate initial block for root directory.");
    }
    inode_table[root_inode_idx].direct_blocks[0] = *block_num_opt;

    log_operation("INIT", "/", "SUCCESS", "File system initialized");
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
            return std::nullopt;
        }
    }
    
    if (parent_inode_idx) *parent_inode_idx = p_idx;
    if (child_name) *child_name = parts.back();

    return current_inode_idx;
}

bool FileSystemManager::create_directory(const std::string& path, uint16_t permissions) {
    if (path == "/" || find_inode_by_path(path)) return false;

    auto parts = split_path(path);
    if(parts.empty()) return false;
    std::string new_dir_name = parts.back();
    parts.pop_back();
    
    std::string parent_path = "/";
    if (!parts.empty()) {
        parent_path += parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            parent_path += "/" + parts[i];
        }
    }

    auto parent_inode_opt = find_inode_by_path(parent_path);
    if (!parent_inode_opt) return false;

    auto new_inode_idx_opt = allocate_inode();
    if (!new_inode_idx_opt) return false;
    
    auto block_num_opt = allocate_block();
    if (!block_num_opt) {
        free_inode(*new_inode_idx_opt);
        return false;
    }

    inode_table[*new_inode_idx_opt] = {InodeType::DIRECTORY, 0, 1, {*block_num_opt}, permissions, std::time(nullptr), std::time(nullptr)};

    if (!add_dir_entry(*parent_inode_opt, *new_inode_idx_opt, new_dir_name)) {
        free_inode(*new_inode_idx_opt);
        free_block(*block_num_opt);
        return false;
    }
    log_operation("CREATE_DIR", path, "SUCCESS");
    return true;
}

bool FileSystemManager::create_file(const std::string& path, const std::string& content, uint16_t permissions) {
    if (path == "/" || find_inode_by_path(path)) return false;

    auto parts = split_path(path);
    if(parts.empty()) return false;

    std::string filename = parts.back();
    parts.pop_back();

    std::string parent_path = "/";
    if (!parts.empty()) {
        parent_path += parts[0];
        for (size_t i = 1; i < parts.size(); ++i) parent_path += "/" + parts[i];
    }
    
    auto parent_inode_opt = find_inode_by_path(parent_path);
    if (!parent_inode_opt) return false;

    auto new_inode_idx_opt = allocate_inode();
    if (!new_inode_idx_opt) return false;
    
    // 1. Setup Inode
    auto& new_inode = inode_table[*new_inode_idx_opt];
    new_inode.type = InodeType::FILE;
    new_inode.permissions = permissions;
    new_inode.size = static_cast<uint32_t>(content.size());
    new_inode.created_at = std::time(nullptr);
    new_inode.modified_at = new_inode.created_at;

    // 2. Allocate blocks and write content
    size_t remaining = content.size();
    size_t offset = 0;
    std::vector<uint32_t> new_blocks;
    for (int i = 0; i < DIRECT_BLOCKS && remaining > 0; ++i) {
        auto block_num_opt = allocate_block();
        if (!block_num_opt) {
            // Cleanup partially allocated blocks before failing
            for(uint32_t b : new_blocks) free_block(b);
            free_inode(*new_inode_idx_opt);
            return false;
        }
        new_blocks.push_back(*block_num_opt);
        uint64_t block_addr = disk_base_address + *block_num_opt * BLOCK_SIZE;
        size_t to_write = std::min(remaining, (size_t)BLOCK_SIZE);
        memory_manager.write_memory(block_addr, content.substr(offset, to_write));
        remaining -= to_write;
        offset += to_write;
    }
    // Commit new blocks to inode
    for(size_t i = 0; i < new_blocks.size(); ++i) {
        new_inode.direct_blocks[i] = new_blocks[i];
    }


    // 3. Add to parent directory
    if (!add_dir_entry(*parent_inode_opt, *new_inode_idx_opt, filename)) {
        // Cleanup if adding entry fails
        for(uint32_t b : new_blocks) free_block(b);
        free_inode(*new_inode_idx_opt);
        return false;
    }
    
    log_operation("CREATE_FILE", path, "SUCCESS");
    return true;
}

std::optional<FileContent> FileSystemManager::read_file(const std::string& path) {
    auto inode_idx_opt = find_inode_by_path(path);
    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::FILE) return std::nullopt;
    
    const auto& inode = inode_table[*inode_idx_opt];
    std::string content_buffer;
    content_buffer.reserve(inode.size);
    size_t remaining = inode.size;

    for (int i = 0; i < DIRECT_BLOCKS && remaining > 0; ++i) {
        uint32_t block_num = inode.direct_blocks[i];
        if (block_num == 0) break;
        uint64_t block_addr = disk_base_address + block_num * BLOCK_SIZE;
        size_t to_read = std::min(remaining, (size_t)BLOCK_SIZE);
        content_buffer += memory_manager.read_memory(block_addr, to_read);
        remaining -= to_read;
    }

    return FileContent{path, content_buffer, inode.permissions, inode.size, inode.created_at, inode.modified_at};
}

bool FileSystemManager::write_file(const std::string& path, const std::string& content, bool append) {
    uint32_t parent_idx;
    std::string child_name;
    auto inode_idx_opt = find_inode_by_path(path, &parent_idx, &child_name);
    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::FILE) return false;
    
    auto& inode = inode_table[*inode_idx_opt];
    std::string final_content = content;

    if (append) {
        auto existing_content_opt = read_file(path);
        if(existing_content_opt) final_content = existing_content_opt->content + content;
    }
    
    for (int i = 0; i < DIRECT_BLOCKS; ++i) {
        if (inode.direct_blocks[i] != 0) free_block(inode.direct_blocks[i]);
        inode.direct_blocks[i] = 0;
    }

    inode.size = static_cast<uint32_t>(final_content.size());
    size_t remaining = inode.size;
    size_t offset = 0;
    
    for (int i = 0; i < DIRECT_BLOCKS && remaining > 0; ++i) {
        auto block_num_opt = allocate_block();
        if (!block_num_opt) { /* Error handling: cleanup allocated blocks */ return false; }
        inode.direct_blocks[i] = *block_num_opt;
        uint64_t block_addr = disk_base_address + *block_num_opt * BLOCK_SIZE;
        size_t to_write = std::min(remaining, (size_t)BLOCK_SIZE);
        memory_manager.write_memory(block_addr, final_content.substr(offset, to_write));
        remaining -= to_write;
        offset += to_write;
    }
    
    inode.modified_at = std::time(nullptr);
    return true;
}

bool FileSystemManager::delete_file(const std::string& path) {
    uint32_t parent_inode_idx;
    std::string filename;
    auto inode_idx_opt = find_inode_by_path(path, &parent_inode_idx, &filename);

    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::FILE) return false;

    for(int i=0; i<DIRECT_BLOCKS; ++i) {
        if(inode_table[*inode_idx_opt].direct_blocks[i] != 0) {
            free_block(inode_table[*inode_idx_opt].direct_blocks[i]);
        }
    }
    
    free_inode(*inode_idx_opt);
    return remove_dir_entry(parent_inode_idx, filename);
}

FsDeleteResult FileSystemManager::delete_directory(const std::string& path, bool recursive) {
    if (path == "/") return FsDeleteResult::DirectoryNotEmpty; // Cannot delete root
    
    uint32_t parent_inode_idx;
    std::string dirname;
    auto inode_idx_opt = find_inode_by_path(path, &parent_inode_idx, &dirname);

    if (!inode_idx_opt) return FsDeleteResult::NotFound;
    if (inode_table[*inode_idx_opt].type != InodeType::DIRECTORY) return FsDeleteResult::IsFile;

    auto entries = get_dir_entries(*inode_idx_opt);
    if (!entries.empty() && !recursive) return FsDeleteResult::DirectoryNotEmpty;

    for (const auto& entry : entries) {
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;
        
        std::string child_path = path;
        if (child_path.back() != '/') {
            child_path += '/';
        }
        child_path += entry.name;
        
        if (inode_table[entry.inode_num].type == InodeType::DIRECTORY) {
            auto result = delete_directory(child_path, true);
            if (result != FsDeleteResult::Success) return result; // Propagate error
        } else {
            if (!delete_file(child_path)) {
                // This case is tricky. It could mean the file wasn't found,
                // which would be an inconsistency. We'll treat as a generic failure.
                return FsDeleteResult::NotFound; // Or a new error code
            }
        }
    }
    
    // Free the directory's own data block and inode
    free_block(inode_table[*inode_idx_opt].direct_blocks[0]);
    free_inode(*inode_idx_opt);
    
    // Remove from parent
    if (remove_dir_entry(parent_inode_idx, dirname)) {
        return FsDeleteResult::Success;
    } else {
        // This indicates an inconsistency, parent doesn't contain the entry.
        return FsDeleteResult::NotFound;
    }
}

std::optional<std::vector<DirectoryContent>> FileSystemManager::list_directory(const std::string& path) {
    auto inode_idx_opt = find_inode_by_path(path);
    if (!inode_idx_opt || inode_table[*inode_idx_opt].type != InodeType::DIRECTORY) return std::nullopt;
    
    std::vector<DirectoryContent> contents;
    for (const auto& entry : get_dir_entries(*inode_idx_opt)) {
        if (entry.inode_num != 0) {
            const auto& inode = inode_table[entry.inode_num];
            contents.push_back({
                entry.name,
                (inode.type == InodeType::DIRECTORY) ? "directory" : "file",
                inode.size,
                inode.permissions,
                inode.created_at,
                inode.modified_at
            });
        }
    }
    return contents;
}

std::vector<DirectoryEntry> FileSystemManager::get_dir_entries(uint32_t inode_idx) {
    const auto& inode = inode_table[inode_idx];
    if (inode.type != InodeType::DIRECTORY) return {};
    
    std::vector<DirectoryEntry> entries;
    std::string data = memory_manager.read_memory(disk_base_address + inode.direct_blocks[0] * BLOCK_SIZE, inode.size);
    
    DirectoryEntry entry;
    for(size_t offset = 0; offset < inode.size; offset += sizeof(DirectoryEntry)) {
        memcpy(&entry, data.c_str() + offset, sizeof(DirectoryEntry));
        entries.push_back(entry);
    }
    return entries;
}

bool FileSystemManager::add_dir_entry(uint32_t parent_inode_idx, uint32_t new_inode_idx, const std::string& name) {
    auto& parent_inode = inode_table[parent_inode_idx];
    if (parent_inode.type != InodeType::DIRECTORY || parent_inode.size + sizeof(DirectoryEntry) > BLOCK_SIZE) return false;

    DirectoryEntry new_entry;
    new_entry.inode_num = new_inode_idx;
    strncpy(new_entry.name, name.c_str(), sizeof(new_entry.name) - 1);
    new_entry.name[sizeof(new_entry.name) - 1] = '\0';
    
    uint64_t block_addr = disk_base_address + parent_inode.direct_blocks[0] * BLOCK_SIZE;
    
    std::string entry_data(reinterpret_cast<const char*>(&new_entry), sizeof(DirectoryEntry));
    memory_manager.write_memory(block_addr + parent_inode.size, entry_data);
    
    parent_inode.size += sizeof(DirectoryEntry);
    parent_inode.modified_at = std::time(nullptr);
    return true;
}

bool FileSystemManager::remove_dir_entry(uint32_t parent_inode_idx, const std::string& name) {
    auto& parent_inode = inode_table[parent_inode_idx];
    if (parent_inode.type != InodeType::DIRECTORY) return false;

    auto entries = get_dir_entries(parent_inode_idx);
    auto original_size = entries.size();
    
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const DirectoryEntry& entry){
        return strcmp(entry.name, name.c_str()) == 0;
    }), entries.end());

    if (entries.size() == original_size) return false; // Not found

    uint64_t block_addr = disk_base_address + parent_inode.direct_blocks[0] * BLOCK_SIZE;
    parent_inode.size = 0; // Will be recalculated
    for(const auto& entry : entries) {
        std::string entry_data(reinterpret_cast<const char*>(&entry), sizeof(DirectoryEntry));
        memory_manager.write_memory(block_addr + parent_inode.size, entry_data);
        parent_inode.size += sizeof(DirectoryEntry);
    }
    parent_inode.modified_at = std::time(nullptr);
    return true;
}

void FileSystemManager::log_operation(const std::string& operation, const std::string& path, const std::string& status, const std::string& details) {
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
    FileSystemStatus status = {};
    status.total_space = DISK_SIZE_BYTES;
    for(const auto& used : block_bitmap) {
        if(used) status.used_space += BLOCK_SIZE;
    }
    status.free_space = status.total_space - status.used_space;
    status.total_files = 0;
    status.total_directories = 0;
    for(const auto& inode : inode_table) {
        if(inode.type == InodeType::FILE) status.total_files++;
        else if (inode.type == InodeType::DIRECTORY) status.total_directories++;
    }
    return status;
}

std::vector<FileSystemLog> FileSystemManager::get_filesystem_logs(const std::string& start_time, const std::string& end_time, const std::string& operation_type) {
    // Implementation for log filtering if needed
    return operation_logs;
}