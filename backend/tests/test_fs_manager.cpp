#include "fs/fs_manager.h"
#include "test_common.h"
#include <vector>
#include <cstring> // For strcmp

// Since we can't and shouldn't instantiate MemoryManager here
// we just test the FileSystemManager in isolation.

void test_initialization() {
    std::cout << "  - Testing Initialization..." << std::endl;
    FileSystemManager fsm;
    fsm.initialize(); // The new way to set up the root directory etc.

    auto status = fsm.get_filesystem_status();
    ASSERT_EQUAL(status.total_space, DISK_SIZE_BYTES);
    ASSERT_TRUE(status.used_space > 0); // Some space used by root inode and root dir block
    ASSERT_EQUAL(status.total_directories, 1); // Root directory
    ASSERT_EQUAL(status.total_files, 0);

    // Verify root inode exists at index 0
    auto root_inode_num_opt = fsm.find_inode_by_path("/");
    ASSERT_TRUE(root_inode_num_opt.has_value());
    ASSERT_EQUAL(root_inode_num_opt.value(), 0);
    
    std::cout << "    ...PASSED" << std::endl;
}

void test_create_and_find_directory() {
    std::cout << "  - Testing Directory Creation and Finding..." << std::endl;
    FileSystemManager fsm;
    fsm.initialize();

    // Create a directory
    uint16_t permissions = 0755;
    auto create_result = fsm.create_directory("/home", permissions);
    ASSERT_EQUAL(static_cast<int>(create_result), static_cast<int>(FsCreateResult::Success));

    // Find it
    auto home_inode_num_opt = fsm.find_inode_by_path("/home");
    ASSERT_TRUE(home_inode_num_opt.has_value());
    ASSERT_TRUE(home_inode_num_opt.value() > 0); // Should not be root

    // List root directory to confirm
    auto root_content_opt = fsm.list_directory("/");
    ASSERT_TRUE(root_content_opt.has_value());
    bool found_home = false;
    for (const auto& entry : root_content_opt.value()) {
        if (entry.name == "home" && entry.type == "directory") {
            found_home = true;
            break;
        }
    }
    ASSERT_TRUE(found_home);
    
    // Create nested directory
    create_result = fsm.create_directory("/home/user", permissions);
    ASSERT_EQUAL(static_cast<int>(create_result), static_cast<int>(FsCreateResult::Success));

    // Find nested directory
    auto user_inode_num_opt = fsm.find_inode_by_path("/home/user");
    ASSERT_TRUE(user_inode_num_opt.has_value());

    // List /home to see 'user'
    auto home_content_opt = fsm.list_directory("/home");
    ASSERT_TRUE(home_content_opt.has_value());
    bool found_user = false;
    for (const auto& entry : home_content_opt.value()) {
        if (entry.name == "user" && entry.type == "directory") {
            found_user = true;
            break;
        }
    }
    ASSERT_TRUE(found_user);

    std::cout << "    ...PASSED" << std::endl;
}


void test_create_and_read_file() {
    std::cout << "  - Testing File Creation and Reading..." << std::endl;
    FileSystemManager fsm;
    fsm.initialize();
    fsm.create_directory("/tmp", 0755);

    uint64_t file_size = 12345;
    uint16_t permissions = 0644;
    std::string file_path = "/tmp/test.txt";

    // Create file
    auto create_result = fsm.create_file(file_path, file_size, permissions);
    ASSERT_EQUAL(static_cast<int>(create_result), static_cast<int>(FsCreateResult::Success));
    
    // Read file
    auto file_content_opt = fsm.read_file(file_path);
    ASSERT_TRUE(file_content_opt.has_value());
    
    auto content = file_content_opt.value();
    ASSERT_TRUE(content.path == file_path);
    ASSERT_EQUAL(content.simulated_size, file_size);
    ASSERT_EQUAL(content.permissions, permissions);
    // The dummy content check
    ASSERT_TRUE(content.content.find(std::to_string(file_size)) != std::string::npos);

    auto status = fsm.get_filesystem_status();
    ASSERT_EQUAL(status.total_files, 1);
    
    std::cout << "    ...PASSED" << std::endl;
}

void test_deletion() {
    std::cout << "  - Testing Deletion..." << std::endl;
    FileSystemManager fsm;
    fsm.initialize();
    fsm.create_directory("/d", 0755);
    fsm.create_file("/d/f", 100, 0644);

    // Fail to delete non-empty directory
    auto delete_res = fsm.delete_directory("/d", false);
    ASSERT_EQUAL(static_cast<int>(delete_res), static_cast<int>(FsDeleteResult::DirectoryNotEmpty));

    // Delete the file first
    bool file_deleted = fsm.delete_file("/d/f");
    ASSERT_TRUE(file_deleted);

    // Now deleting directory should succeed
    delete_res = fsm.delete_directory("/d", false);
    ASSERT_EQUAL(static_cast<int>(delete_res), static_cast<int>(FsDeleteResult::Success));

    // Verify it's gone
    auto find_opt = fsm.find_inode_by_path("/d");
    ASSERT_FALSE(find_opt.has_value());
    
    std::cout << "    ...PASSED" << std::endl;
}

void run_fs_manager_tests() {
    test_initialization();
    test_create_and_find_directory();
    test_create_and_read_file();
    test_deletion();
} 