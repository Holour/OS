#ifndef INODE_H
#define INODE_H

#include <cstdint>
#include <ctime>
#include <vector>
#include <string>

// 模拟磁盘块大小
const uint32_t BLOCK_SIZE = 512;
// Inode中直接数据块指针的数量
const int DIRECT_BLOCKS = 12;

enum class InodeMode {
    UNALLOCATED,
    FILE,
    DIRECTORY
};

// 索引节点
struct Inode {
    InodeMode mode = InodeMode::UNALLOCATED;
    uint32_t size = 0;      // 文件大小（字节）
    uint32_t block_count = 0; // 占用的块数
    time_t last_modified_time;

    uint32_t direct_blocks[DIRECT_BLOCKS] = {0}; // 指向数据块的指针
    // 为了简化，我们暂时不实现间接块
    // uint32_t indirect_block = 0; 
};

// 目录项结构
struct DirectoryEntry {
    char name[28]; // 文件名或目录名
    uint32_t inode_number;
};

#endif //INODE_H 