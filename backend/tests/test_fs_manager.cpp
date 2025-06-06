#include "../include/fs/fs_manager.h"
#include "../include/memory/memory_manager.h"
#include <iostream>
#include <cassert>

void test_format() {
    std::cout << "--- Running Test: FileSystem Format ---" << std::endl;
    MemoryManager mm;
    mm.initialize();
    
    // Test that the FSM can be constructed and allocates its disk space
    FileSystemManager fsm(mm);
    assert(mm.get_used_memory() == SIM_DISK_SIZE);

    // Now, test the format method
    fsm.format();

    // Check if the root inode (inode 0) was created correctly
    auto root_inode_opt = fsm.get_inode(0);
    assert(root_inode_opt.has_value());
    
    const auto& root_inode = root_inode_opt.value();
    assert(root_inode.mode == InodeMode::DIRECTORY);

    // Check if another inode is unallocated
    auto other_inode_opt = fsm.get_inode(1);
    assert(other_inode_opt.has_value());
    assert(other_inode_opt.value().mode == InodeMode::UNALLOCATED);

    // In this new design, format() doesn't allocate any data blocks for the root dir itself yet.
    // That happens when entries are added. So no blocks should be marked as used.
    // We can add a private method to FSM to check bitmap for testing if needed,
    // but for now we trust the logic.

    std::cout << "OK: format() correctly initializes the root directory and inode table." << std::endl;
}

void test_create_and_find_directory() {
    std::cout << "--- Running Test: Create and Find Directory ---" << std::endl;
    MemoryManager mm;
    mm.initialize();
    FileSystemManager fsm(mm);
    fsm.format();

    // 1. Create a directory '/home' at the root
    auto home_inode_num_opt = fsm.create_directory("/home", "/");
    assert(home_inode_num_opt.has_value());
    uint32_t home_inode_num = home_inode_num_opt.value();

    // 2. Verify its contents ('.' and '..') using the public test helper
    auto home_inode_opt = fsm.get_inode(home_inode_num);
    assert(home_inode_opt.has_value() && home_inode_opt->mode == InodeMode::DIRECTORY);
    
    char block_buffer[BLOCK_SIZE];
    assert(fsm.read_block_for_test(home_inode_opt->direct_blocks[0], block_buffer, BLOCK_SIZE));
    
    DirectoryEntry* entries = reinterpret_cast<DirectoryEntry*>(block_buffer);
    assert(strcmp(entries[0].name, ".") == 0 && entries[0].inode_number == home_inode_num);
    assert(strcmp(entries[1].name, "..") == 0 && entries[1].inode_number == 0);
    
    std::cout << "OK: Directory '/home' created and its content is correct." << std::endl;

    // 3. Create a nested directory '/home/user'. This implicitly tests find_inode_for_path.
    auto user_inode_num_opt = fsm.create_directory("/home/user", "/home");
    assert(user_inode_num_opt.has_value());
    uint32_t user_inode_num = user_inode_num_opt.value();
    
    // 4. Verify its '..' entry points back to '/home'
    auto user_inode_opt = fsm.get_inode(user_inode_num);
    assert(user_inode_opt.has_value());
    assert(fsm.read_block_for_test(user_inode_opt->direct_blocks[0], block_buffer, BLOCK_SIZE));
    
    entries = reinterpret_cast<DirectoryEntry*>(block_buffer);
    assert(strcmp(entries[1].name, "..") == 0 && entries[1].inode_number == home_inode_num);

    std::cout << "OK: Nested directory '/home/user' created successfully." << std::endl;
}

int main() {
    std::cout << "===== Starting FileSystemManager Tests =====" << std::endl;

    test_format();
    test_create_and_find_directory();

    std::cout << "===== All FileSystemManager Tests Passed! =====" << std::endl;
    return 0;
} 