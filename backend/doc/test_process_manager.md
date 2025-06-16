# 单元测试说明：`test_process_manager.cpp`

本文档详细说明了 `test_process_manager.cpp` 文件中的单元测试，这些测试旨在验证 `ProcessManager` 类的功能，包括进程的生命周期管理和调度。

## 概述

`test_process_manager.cpp` 包含一系列针对进程管理器的测试用例。这些测试依赖于 `MemoryManager` 的实例来模拟内存分配，覆盖了进程的成功创建、因内存不足（OOM）导致的创建失败、进程终止以及 FCFS 调度算法的正确性。

## 测试用例详情

### 1. `test_pm_create_process_success()`

- **目的**: 验证一个新进程能否被成功创建。
- **测试步骤**:
    1. 初始化 `MemoryManager` 和 `ProcessManager`。
    2. 记录初始的已用内存量。
    3. 调用 `create_process()` 创建一个新进程，并指定其名称、所需内存、优先级和父进程ID。
    4. 验证操作返回了一个有效的进程ID (PID)。
    5. 通过返回的 PID 获取进程控制块 (PCB)，并验证其不为空。
    6. 验证新进程的初始状态为 `READY`。
    7. 验证进程列表中的进程总数为 1。
    8. 验证已用内存量在进程创建后有所增加。
- **断言**:
    - `ASSERT_TRUE(pid_opt.has_value())`
    - `ASSERT_NOT_NULL(pcb_ptr)`
    - `ASSERT_EQUAL(static_cast<int>(pcb_ptr->state), static_cast<int>(ProcessState::READY))`
    - `ASSERT_EQUAL(pm.get_all_processes().size(), 1)`
    - `ASSERT_TRUE(mm.get_used_memory() > initial_used_memory)`

### 2. `test_pm_create_process_oom()`

- **目的**: 验证当内存不足时，进程创建会失败。
- **测试步骤**:
    1. 初始化 `MemoryManager` 和 `ProcessManager`。
    2. 尝试创建一个需要比系统总内存还多的进程。
    3. 验证 `create_process()` 操作失败，没有返回 PID。
    4. 验证进程列表中没有添加任何新进程。
- **断言**:
    - `ASSERT_FALSE(pid_opt.has_value())`
    - `ASSERT_EQUAL(pm.get_all_processes().size(), 0)`

### 3. `test_pm_terminate_process()`

- **目的**: 验证进程终止功能，包括资源释放。
- **测试步骤**:
    1. 初始化 `MemoryManager` 和 `ProcessManager`。
    2. 创建一个进程并记录其 PID。
    3. 记录终止前的已用内存量。
    4. 调用 `terminate_process()` 终止该进程，并验证操作返回 `true`。
    5. 尝试再次获取该进程的 PCB，并验证其为空，表示进程已被移除。
    6. 验证已用内存量在进程终止后减少。
    7. 尝试终止一个不存在的进程ID (如 9999)，并验证操作失败，返回 `false`。
- **断言**:
    - `ASSERT_TRUE(result)`: 验证成功终止。
    - `ASSERT_NULL(pm.get_process(pid_to_terminate))`: 验证进程已被移除。
    - `ASSERT_TRUE(mm.get_used_memory() < memory_before_termination)`: 验证内存被释放。
    - `ASSERT_FALSE(non_existent_result)`: 验证终止不存在的进程会失败。

### 4. `test_pm_scheduler_fcfs()`

- **目的**: 验证"先来先服务"（First-Come, First-Served, FCFS）调度算法的正确性。
- **测试步骤**:
    1. 初始化 `MemoryManager` 和 `ProcessManager`，并将调度算法设置为 FCFS。
    2. 按顺序创建两个进程 (A 和 B)。
    3. 调用 `schedule()` 第一次，并验证返回的是进程 A 的 PCB，且其状态变为 `RUNNING`。
    4. 调用 `schedule()` 第二次，并验证返回的是进程 B 的 PCB，且其状态变为 `RUNNING`。
    5. 再次调用 `schedule()`，此时就绪队列应为空，验证返回 `nullptr`。
- **断言**:
    - `ASSERT_EQUAL(scheduled_p1->pid, pidA_opt.value())`: 验证第一个调度的是 A。
    - `ASSERT_EQUAL(scheduled_p2->pid, pidB_opt.value())`: 验证第二个调度的是 B。
    - `ASSERT_EQUAL(static_cast<int>(scheduled_p->state), static_cast<int>(ProcessState::RUNNING))`: 验证进程状态变更。
    - `ASSERT_NULL(should_be_null)`: 验证就绪队列为空时调度返回空。

---

所有测试都通过 `run_process_manager_tests()` 函数统一调用。 