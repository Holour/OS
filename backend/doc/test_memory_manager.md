# 单元测试说明：`test_memory_manager.cpp`

本文档详细说明了 `test_memory_manager.cpp` 文件中的单元测试，这些测试旨在验证 `MemoryManager` 类的连续内存管理功能。

## 概述

`test_memory_manager.cpp` 包含一系列针对内存管理器的测试用例，覆盖了从初始化、简单分配、内存溢出（OOM）到复杂的内存释放与合并场景。

## 测试用例详情

### 1. `test_mm_initialization()`

- **目的**: 验证 `MemoryManager` 在 `initialize()` 调用后是否处于正确的初始状态。
- **测试步骤**:
    1. 创建一个 `MemoryManager` 实例并调用 `initialize()`。
    2. 检查已用内存是否为 0。
    3. 检查总内存量是否大于 0。
    4. 验证初始时只有一个空闲内存块。
    5. 验证这个唯一的空闲块的基地址为 0，并且其大小等于总内存量。
- **断言**:
    - `ASSERT_EQUAL(mm.get_used_memory(), 0)`
    - `ASSERT_TRUE(mm.get_total_memory() > 0)`
    - `ASSERT_EQUAL(mm.get_free_blocks().size(), 1)`
    - `ASSERT_EQUAL(mm.get_free_blocks().front().base_address, 0)`
    - `ASSERT_EQUAL(mm.get_free_blocks().front().size, total_mem)`

### 2. `test_mm_simple_allocation()`

- **目的**: 验证一次简单的内存分配操作是否正确。
- **测试步骤**:
    1. 初始化 `MemoryManager`。
    2. 请求分配一块大小为 1024 字节的内存。
    3. 验证分配操作成功，并返回一个有效的内存块。
    4. 验证返回的内存块基地址为 0。
    5. 验证已用内存量等于所分配的大小 (1024)。
    6. 验证剩余的空闲内存块的基地址已更新，并且大小已相应减少。
- **断言**:
    - `ASSERT_TRUE(blk_opt.has_value())`
    - `ASSERT_EQUAL(blk_opt->base_address, 0)`
    - `ASSERT_EQUAL(mm.get_used_memory(), size)`
    - `ASSERT_EQUAL(mm.get_free_blocks().front().base_address, size)`
    - `ASSERT_EQUAL(mm.get_free_blocks().front().size, total_mem - size)`

### 3. `test_mm_allocation_oom()`

- **目的**: 验证当请求的内存量超过可用内存时，系统能否正确处理"内存溢出"（Out of Memory）情况。
- **测试步骤**:
    1. 初始化 `MemoryManager`。
    2. 请求分配一块比总内存量还大的内存。
    3. 验证分配操作失败，返回一个空的可选项 (`std::optional` a `std::nullopt`)。
- **断言**:
    - `ASSERT_FALSE(blk_oom.has_value())`

### 4. `test_mm_free_and_merge()`

- **目的**: 验证内存释放功能以及相邻空闲块的合并（coalescing）逻辑。
- **测试步骤**:
    1. **分配**: 初始化 `MemoryManager` 并连续分配三块内存（大小分别为 100, 200, 300）。
    2. **释放中间块**: 释放第二块内存 (200 字节)。此时，它应该成为一个新的独立空闲块，不与任何其他块合并。验证空闲块数量为 2。
    3. **释放第一块**: 释放第一块内存 (100 字节)。此时，它应该与紧随其后的空闲块（原第二块）合并。验证空闲块数量仍为 2，但第一个空闲块的大小变为 300。
    4. **释放最后一块**: 释放第三块内存 (300 字节)。此时，它应该与主空闲块合并，最终使整个内存空间恢复为一个单一的、完整的空闲块。验证已用内存为 0，空闲块数量为 1，且大小等于总内存。
- **断言**:
    - `ASSERT_EQUAL(mm.get_used_memory(), ...)`: 在不同阶段验证已用内存的正确性。
    - `ASSERT_EQUAL(mm.get_free_blocks().size(), ...)`: 在不同阶段验证空闲块数量的正确性。
    - `ASSERT_EQUAL(mm.get_free_blocks().front().size, ...)`: 验证合并后空闲块大小的正确性。

---

所有测试都通过 `run_memory_manager_tests()` 函数统一调用。 