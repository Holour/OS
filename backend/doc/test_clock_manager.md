# 单元测试说明：`test_clock_manager.cpp`

本文档详细说明了 `test_clock_manager.cpp` 文件中的单元测试，这些测试旨在验证 `ClockManager` 类的功能和稳定性。

## 概述

`test_clock_manager.cpp` 包含一系列针对时钟管理器的测试用例，覆盖了从初始状态验证、时间流逝到定时器管理等核心功能。

## 测试用例详情

### 1. `test_initial_state()`

- **目的**: 验证 `ClockManager` 在实例化后的初始状态是否正确。
- **测试步骤**:
    1. 创建一个 `ClockManager` 实例。
    2. 检查初始的 `ticks` 是否为 0。
    3. 检查初始的 `uptime` (系统运行时间) 是否为一个接近于 0 的极小值（小于 100 毫秒）。
- **断言**:
    - `ASSERT_EQUAL(cm.get_ticks(), 0)`
    - `ASSERT_TRUE(cm.get_uptime_ms() < 100)`

### 2. `test_time_progression()`

- **目的**: 验证 `ClockManager` 启动后，时间和 `ticks` 是否能正确地向前推进。
- **测试步骤**:
    1. 创建一个 `ClockManager` 实例并调用 `start()` 方法。
    2. 记录当前的 `uptime` 和 `ticks`。
    3. 使当前线程休眠 150 毫秒。
    4. 再次获取 `uptime` 和 `ticks`。
    5. 比较休眠前后的值，验证时间是否正确增加。
- **断言**:
    - `ASSERT_TRUE(later_uptime > initial_uptime)`: 验证运行时间增加了。
    - `ASSERT_TRUE(later_uptime >= 150)`: 验证运行时间的增量至少是休眠的时间。
    - `ASSERT_TRUE(later_ticks > initial_ticks)`: 验证 `ticks` 也随时间增加了。

### 3. `test_timer_management()`

- **目的**: 验证 `ClockManager` 的定时器管理功能，包括设置一次性定时器和周期性定时器。
- **测试步骤**:
    1. 创建一个 `ClockManager` 实例并调用 `start()` 方法。
    2. 调用 `set_timer()` 方法设置一个 200 毫秒后触发的一次性定时器。
    3. 验证是否成功返回了定时器 ID (ID 为 1)。
    4. 调用 `set_timer()` 方法设置一个每 100 毫秒触发一次的周期性定时器，首次触发在 300 毫秒后。
    5. 验证是否成功返回了新的定时器 ID (ID 为 2)。
    6. 尝试用一个无效的间隔（0）来设置定时器，并验证它是否返回了一个新的、不同于之前定时器的 ID。
- **断言**:
    - `ASSERT_TRUE(timer1_id.has_value())` 和 `ASSERT_EQUAL(timer1_id.value(), 1)`
    - `ASSERT_TRUE(timer2_id.has_value())` 和 `ASSERT_EQUAL(timer2_id.value(), 2)`
    - `ASSERT_TRUE(timer3_id.has_value())` 和 `ASSERT_NOT_EQUAL(timer3_id.value(), timer2_id.value())`

---

所有测试都通过 `run_clock_manager_tests()` 函数统一调用。 