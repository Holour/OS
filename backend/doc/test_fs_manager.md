# 单元测试说明：`test_fs_manager.cpp`

本文档详细说明了 `test_fs_manager.cpp` 文件中的单元测试，这些测试旨在验证 `FileSystemManager` 类的功能和正确性。

## 概述

`test_fs_manager.cpp` 包含一系列针对文件系统管理器的测试用例，覆盖了文件系统的初始化、目录和文件的创建、查找、读取和删除等核心功能。

## 测试用例详情

### 1. `test_initialization()`

- **目的**: 验证 `FileSystemManager` 在 `initialize()` 调用后是否正确地设置了文件系统的初始状态。
- **测试步骤**:
    1. 创建一个 `FileSystemManager` 实例并调用 `initialize()`。
    2. 获取文件系统状态。
    3. 验证总空间是否等于预设的磁盘大小 (`DISK_SIZE_BYTES`)。
    4. 验证已用空间大于零（因为根目录的 `inode` 和数据块会占用空间）。
    5. 验证初始时只有一个目录（根目录）和零个文件。
    6. 验证能通过路径 `/` 找到根目录的 `inode`，且其 `inode` 编号为 0。
- **断言**:
    - `ASSERT_EQUAL(status.total_space, DISK_SIZE_BYTES)`
    - `ASSERT_TRUE(status.used_space > 0)`
    - `ASSERT_EQUAL(status.total_directories, 1)`
    - `ASSERT_EQUAL(status.total_files, 0)`
    - `ASSERT_TRUE(root_inode_num_opt.has_value())`
    - `ASSERT_EQUAL(root_inode_num_opt.value(), 0)`

### 2. `test_create_and_find_directory()`

- **目的**: 验证创建单个目录和嵌套目录的功能，并确保能正确地找到它们。
- **测试步骤**:
    1. 初始化文件系统。
    2. 在根目录下创建一个名为 `home` 的目录，并验证操作成功。
    3. 通过路径 `/home` 查找该目录，验证其存在。
    4. 列出根目录 `/` 的内容，确认 `home` 目录在列表中。
    5. 在 `/home` 下创建一个名为 `user` 的嵌套目录，并验证操作成功。
    6. 通过路径 `/home/user` 查找该嵌套目录，验证其存在。
    7. 列出 `/home` 目录的内容，确认 `user` 目录在列表中。
- **断言**:
    - `ASSERT_EQUAL(static_cast<int>(create_result), static_cast<int>(FsCreateResult::Success))`
    - `ASSERT_TRUE(home_inode_num_opt.has_value())`
    - `ASSERT_TRUE(found_home)`
    - `ASSERT_TRUE(user_inode_num_opt.has_value())`
    - `ASSERT_TRUE(found_user)`

### 3. `test_create_and_read_file()`

- **目的**: 验证文件的创建和读取功能。
- **测试步骤**:
    1. 初始化文件系统并创建一个 `/tmp` 目录。
    2. 在 `/tmp` 目录下创建一个名为 `test.txt` 的文件，并指定其大小和权限。
    3. 验证文件创建操作成功。
    4. 读取刚刚创建的文件 `/tmp/test.txt`。
    5. 验证读取操作成功，并且返回的文件内容（包括路径、模拟大小、权限和模拟内容）与创建时提供的一致。
    6. 获取文件系统状态，确认总文件数已增加到 1。
- **断言**:
    - `ASSERT_EQUAL(static_cast<int>(create_result), static_cast<int>(FsCreateResult::Success))`
    - `ASSERT_TRUE(file_content_opt.has_value())`
    - `ASSERT_TRUE(content.path == file_path)`
    - `ASSERT_EQUAL(content.simulated_size, file_size)`
    - `ASSERT_EQUAL(content.permissions, permissions)`
    - `ASSERT_EQUAL(status.total_files, 1)`

### 4. `test_deletion()`

- **目的**: 验证文件和目录的删除逻辑，特别是对非空目录的删除限制。
- **测试步骤**:
    1. 初始化文件系统，并创建一个目录 `/d` 和一个文件 `/d/f`。
    2. 尝试删除非空目录 `/d`，并验证操作失败，返回 `DirectoryNotEmpty` 错误。
    3. 先删除文件 `/d/f`，并验证文件删除操作成功。
    4. 再次尝试删除现已为空的目录 `/d`，并验证操作成功。
    5. 尝试通过路径 `/d` 查找该目录，验证其已不存在。
- **断言**:
    - `ASSERT_EQUAL(static_cast<int>(delete_res), static_cast<int>(FsDeleteResult::DirectoryNotEmpty))`
    - `ASSERT_TRUE(file_deleted)`
    - `ASSERT_EQUAL(static_cast<int>(delete_res), static_cast<int>(FsDeleteResult::Success))`
    - `ASSERT_FALSE(find_opt.has_value())`

---

所有测试都通过 `run_fs_manager_tests()` 函数统一调用。 