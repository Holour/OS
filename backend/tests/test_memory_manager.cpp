#include "memory/memory_manager.h"
#include "test_common.h"
#include <iostream>
#include <cassert>
#include <numeric>

void test_mm_initialization() {
    std::cout << "  - Testing MM Initialization..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    ASSERT_EQUAL(mm.get_used_memory(), 0);
    const uint64_t total_mem = mm.get_total_memory();
    ASSERT_TRUE(total_mem > 0);
    ASSERT_EQUAL(mm.get_free_blocks().size(), 1);
    ASSERT_EQUAL(mm.get_free_blocks().front().base_address, 0);
    ASSERT_EQUAL(mm.get_free_blocks().front().size, total_mem);
    std::cout << "    ...PASSED" << std::endl;
}

void test_mm_simple_allocation() {
    std::cout << "  - Testing MM Simple Allocation..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    
    uint64_t size = 1024;
    const uint64_t total_mem = mm.get_total_memory();
    auto blk_opt = mm.allocate(size);
    ASSERT_TRUE(blk_opt.has_value());
    ASSERT_EQUAL(blk_opt->base_address, 0);
    ASSERT_EQUAL(mm.get_used_memory(), size);
    ASSERT_EQUAL(mm.get_free_blocks().front().base_address, size);
    ASSERT_EQUAL(mm.get_free_blocks().front().size, total_mem - size);
    
    std::cout << "    ...PASSED" << std::endl;
}

void test_mm_allocation_oom() {
    std::cout << "  - Testing MM Allocation Out of Memory..." << std::endl;
    MemoryManager mm;
    mm.initialize();

    const uint64_t total_mem = mm.get_total_memory();
    auto blk_oom = mm.allocate(total_mem + 1);
    ASSERT_FALSE(blk_oom.has_value());
    std::cout << "    ...PASSED" << std::endl;
}

void test_mm_free_and_merge() {
    std::cout << "  - Testing MM Free and Merge..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    const uint64_t total_mem = mm.get_total_memory();

    // 1. Allocate three blocks
    auto blk1 = mm.allocate(100);
    auto blk2 = mm.allocate(200);
    auto blk3 = mm.allocate(300);
    
    ASSERT_TRUE(blk1.has_value() && blk2.has_value() && blk3.has_value());
    ASSERT_EQUAL(mm.get_used_memory(), 600);
    
    // 2. Free the middle block (should create a new free block, no merge)
    ASSERT_TRUE(mm.free(blk2->base_address, blk2->size));
    ASSERT_EQUAL(mm.get_used_memory(), 400);
    ASSERT_EQUAL(mm.get_free_blocks().size(), 2);

    // 3. Free the first block (should merge with the next free block)
    ASSERT_TRUE(mm.free(blk1->base_address, blk1->size));
    ASSERT_EQUAL(mm.get_used_memory(), 300);
    ASSERT_EQUAL(mm.get_free_blocks().size(), 2); // Still 2 blocks, but first one is bigger
    ASSERT_EQUAL(mm.get_free_blocks().front().base_address, 0);
    ASSERT_EQUAL(mm.get_free_blocks().front().size, 300);

    // 4. Free the last block (should merge with the main free block)
    ASSERT_TRUE(mm.free(blk3->base_address, blk3->size));
    ASSERT_EQUAL(mm.get_used_memory(), 0);
    ASSERT_EQUAL(mm.get_free_blocks().size(), 1);
    ASSERT_EQUAL(mm.get_free_blocks().front().size, total_mem);

    std::cout << "    ...PASSED" << std::endl;
}

void run_memory_manager_tests() {
    test_mm_initialization();
    test_mm_simple_allocation();
    test_mm_allocation_oom();
    test_mm_free_and_merge();
} 