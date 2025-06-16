# 单元测试说明：`test_device_manager.cpp`

本文档详细说明了 `test_device_manager.cpp` 文件中的单元测试，这些测试旨在验证 `DeviceManager` 类的功能和鲁棒性。

## 概述

`test_device_manager.cpp` 包含一系列针对设备管理器的测试用例，覆盖了从设备初始状态检查、设备的请求与释放，到设备的删除等核心功能。

## 测试用例详情

### 1. `test_initial_device_state()`

- **目的**: 验证 `DeviceManager` 在实例化后，是否正确加载了默认设备，并且这些设备的初始状态是否正确。
- **测试步骤**:
    1. 创建一个 `DeviceManager` 实例。
    2. 获取所有设备列表。
    3. 检查设备列表是否不为空，并验证其中是否包含了预期的默认设备类型（如 "PRINTER" 和 "DISK"）。
    4. 遍历所有设备，确认它们的初始状态都是空闲 (`is_busy` 为 false) 且没有被任何进程占用 (`user_pid` 为空)。
- **断言**:
    - `ASSERT_TRUE(devices.size() > 0)`
    - `ASSERT_TRUE(found_printer)`
    - `ASSERT_TRUE(found_disk)`
    - `ASSERT_FALSE(device.is_busy)`
    - `ASSERT_FALSE(device.user_pid.has_value())`

### 2. `test_device_request_and_release()`

- **目的**: 验证设备可以被进程成功请求和释放，并确保设备状态在这些操作后能被正确更新。
- **测试步骤**:
    1. 创建一个 `DeviceManager` 实例和两个模拟的进程ID (p1, p2)。
    2. 进程 p1 请求一个 "PRINTER" 类型的设备，并验证请求成功。
    3. 进程 p2 请求一个同类型的设备，并验证它也成功获取了一个不同的设备实例。
    4. 检查设备列表，确认有两个打印机设备处于繁忙状态，并且分别被 p1 和 p2 占用。
    5. 进程 p1 释放它所占用的设备，并验证释放操作成功。
    6. 尝试让 p1 释放一个不属于它的设备（p2 的设备），并验证该操作失败。
    7. 再次检查设备列表，确认现在只有一个设备处于繁忙状态，并且 p1 之前占用的设备已经变为空闲。
- **断言**:
    - `ASSERT_TRUE(dev1_id_opt.has_value())` / `ASSERT_TRUE(dev2_id_opt.has_value())`: 验证设备请求成功。
    - `ASSERT_NOT_EQUAL(dev1_id, dev2_id_opt.value())`: 验证不同请求获取不同设备。
    - `ASSERT_EQUAL(busy_printers, 2)`: 验证设备被占用。
    - `ASSERT_TRUE(released)`: 验证成功释放。
    - `ASSERT_FALSE(released)`: 验证非法释放操作失败。
    - `ASSERT_EQUAL(final_busy_count, 1)`: 验证释放后繁忙设备数量正确。
    - `ASSERT_TRUE(dev1_is_free)`: 验证被释放的设备变为空闲。

### 3. `test_device_deletion()`

- **目的**: 验证设备的删除功能，确保只有空闲设备可以被删除。
- **测试步骤**:
    1. 创建一个 `DeviceManager` 实例。
    2. 查找并获取一个当前处于空闲状态的设备 ID。
    3. 调用 `delete_device()` 删除该设备，并验证操作成功。
    4. 再次获取设备列表，验证设备总数已减少，并且被删除的设备已不在列表中。
    5. 请求一个 "DISK" 设备使其变为繁忙状态。
    6. 尝试删除这个繁忙的设备，并验证该操作失败。
- **断言**:
    - `ASSERT_NOT_EQUAL(device_to_delete, -1)`: 确保找到了一个可删除的设备。
    - `ASSERT_TRUE(deleted)`: 验证空闲设备删除成功。
    - `ASSERT_EQUAL(devices_after.size(), devices_before.size() - 1)`: 验证设备总数正确。
    - `ASSERT_FALSE(deleted)`: 验证繁忙设备删除失败。

---

所有测试都通过 `run_device_manager_tests()` 函数统一调用。 