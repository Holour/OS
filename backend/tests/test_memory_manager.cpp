#include "../include/memory/memory_manager.h"
#include "../include/common.h"
#include <iostream>
#include <cassert>
#include <numeric>

void print_free_blocks(const MemoryManager& mm) {
    std::cout << "Free blocks:" << std::endl;
    for (const auto& block : mm.get_free_blocks()) {
        std::cout << "  Base: " << block.base_address << ", Size: " << block.size << std::endl;
    }
}

void test_initialization() {
    std::cout << "--- Running Test: Initialization ---" << std::endl;
    MemoryManager mm;
    mm.initialize();
    assert(mm.get_used_memory() == 0);
    assert(mm.get_total_memory() == MEMORY_SIZE);
    assert(mm.get_free_blocks().size() == 1);
    assert(mm.get_free_blocks().front().base_address == 0);
    assert(mm.get_free_blocks().front().size == MEMORY_SIZE);
    std::cout << "OK: MemoryManager initialized correctly." << std::endl;
}

void test_simple_allocation() {
    std::cout << "--- Running Test: Simple Allocation ---" << std::endl;
    MemoryManager mm;
    mm.initialize();
    
    uint64_t size = 1024;
    auto addr = mm.allocate(size);
    assert(addr.has_value());
    assert(addr.value() == 0);
    assert(mm.get_used_memory() == size);
    assert(mm.get_free_blocks().front().base_address == size);
    assert(mm.get_free_blocks().front().size == MEMORY_SIZE - size);
    
    std::cout << "OK: Simple allocation works." << std::endl;
}

void test_allocation_oom() {
    std::cout << "--- Running Test: Allocation Out of Memory ---" << std::endl;
    MemoryManager mm;
    mm.initialize();

    auto addr = mm.allocate(MEMORY_SIZE + 1);
    assert(!addr.has_value());
    std::cout << "OK: Allocation correctly fails for oversized request." << std::endl;
}

void test_free_and_merge() {
    std::cout << "--- Running Test: Free and Merge ---" << std::endl;
    MemoryManager mm;
    mm.initialize();

    // 1. Allocate three blocks
    auto addr1 = mm.allocate(100);
    auto addr2 = mm.allocate(200);
    auto addr3 = mm.allocate(300);
    
    assert(addr1.has_value() && addr2.has_value() && addr3.has_value());
    assert(mm.get_used_memory() == 600);
    
    // 2. Free the middle block (no merge)
    mm.free(addr2.value(), 200);
    assert(mm.get_used_memory() == 400);
    assert(mm.get_free_blocks().size() == 2); // [Free at 100, size 200], [Free at 600, ...]

    // 3. Free the first block (merge with next)
    mm.free(addr1.value(), 100);
    assert(mm.get_used_memory() == 300);
    assert(mm.get_free_blocks().size() == 2); // [Free at 0, size 300], [Free at 600, ...]
    assert(mm.get_free_blocks().front().base_address == 0);
    assert(mm.get_free_blocks().front().size == 300);

    // 4. Re-allocate addr1 and addr2, then free addr3, then addr2 (merge with prev)
    addr1 = mm.allocate(100);
    addr2 = mm.allocate(200);
    mm.free(addr3.value(), 300); // [Free at 300, size 300], [Free at 600, ...]
    mm.free(addr2.value(), 200); // [Free at 100, size 200], [Free at 300, size 300], ... -> merge
    assert(mm.get_used_memory() == 100);
    
    // 5. Free the last block (merge with both)
    mm.free(addr1.value(), 100);
    assert(mm.get_used_memory() == 0);
    assert(mm.get_free_blocks().size() == 1);
    assert(mm.get_free_blocks().front().size == MEMORY_SIZE);

    std::cout << "OK: Free and merge operations work correctly." << std::endl;
}


int main() {
    std::cout << "===== Starting MemoryManager Tests =====" << std::endl;
    
    test_initialization();
    test_simple_allocation();
    test_allocation_oom();
    test_free_and_merge();

    std::cout << "===== All MemoryManager Tests Passed! =====" << std::endl;

    return 0;
} 