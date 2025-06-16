# 单元测试说明：`test_interrupt_manager.cpp`

本文档详细说明了 `test_interrupt_manager.cpp` 文件中的单元测试，这些测试旨在验证 `InterruptManager` 类的功能。

## 概述

`test_interrupt_manager.cpp` 包含一系列针对中断管理器的测试用例，这些测试依赖于 `MemoryManager` 和 `ProcessManager` 的实例，主要覆盖了中断处理程序的注册和中断触发两个核心功能。

## 依赖

本测试文件需要 `MemoryManager` 和 `ProcessManager` 的实例才能正常工作。测试开始前，会通过一个全局静态实例来初始化这些管理器。

- `static MemoryManager test_mem_manager;`
- `static ProcessManager test_proc_manager(test_mem_manager);`

## 测试用例详情

### 1. `test_handler_registration_real()`

- **目的**: 验证中断处理程序的注册逻辑，包括成功注册和重复注册失败的场景。
- **测试步骤**:
    1. 创建一个 `InterruptManager` 实例。
    2. 为中断向量 `32` 注册一个名为 "TIMER" 的处理程序，并验证注册成功。
    3. 尝试为同一个中断向量 `32` 再次注册一个不同的处理程序 ("IO")，并验证此次注册会失败。
    4. 获取中断向量表，并验证表中只有一个条目，其信息（中断号、类型、优先级）与第一次注册时提供的一致。
- **断言**:
    - `ASSERT_TRUE(registered)`: 验证首次注册成功。
    - `ASSERT_FALSE(registered_again)`: 验证在同一向量上的重复注册失败。
    - `ASSERT_EQUAL(table.size(), 1)`: 验证向量表中只有一个处理程序。
    - `ASSERT_EQUAL(table[0].first, 32)`: 验证中断向量号正确。
    - `ASSERT_TRUE(table[0].second.type == "TIMER")`: 验证处理程序类型正确。
    - `ASSERT_EQUAL(table[0].second.priority, 1)`: 验证优先级正确。

### 2. `test_interrupt_triggering_real()`

- **目的**: 验证中断触发机制的正确性。
- **测试步骤**:
    1. 创建一个 `InterruptManager` 实例。
    2. 尝试触发一个尚未注册任何处理程序的中断向量 `40`，并验证操作失败。
    3. 为中断向量 `33` 注册一个 "SYSCALL" 处理程序。
    4. 触发中断向量 `33`，并附带一些上下文数据。
    5. 验证此次中断触发操作成功。
- **断言**:
    - `ASSERT_FALSE(triggered_nonexistent)`: 验证对未注册向量的触发会失败。
    - `ASSERT_TRUE(triggered_existent)`: 验证对已注册向量的触发会成功。

---

所有测试都通过 `run_interrupt_manager_tests()` 函数统一调用，该函数在执行测试前会确保依赖的 `MemoryManager` 被初始化。 