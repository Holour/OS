# 操作系统模拟器 API 集成测试文档

## 1. 概述

本文档旨在为操作系统模拟器的后端 API 提供一份全面、详细的集成测试说明。集成测试通过模拟客户端（`httplib`）与运行中的 `os_simulator` 服务器进行 HTTP 通信，旨在验证各个核心模块 API 接口的正确性、模块间的协同工作能力以及系统的端到端功能。

**核心目的**:
- **功能验证**: 确保每个 API 端点都按预期工作。
- **协同测试**: 验证跨模块操作（如创建进程会消耗内存）的正确性。
- **边界与异常**: 测试系统在接收到无效输入、请求不存在的资源或处理并发请求时的鲁棒性。

---

## 2. 测试架构与执行

### 2.1. 技术栈
- **HTTP 客户端**: `httplib.h`，一个跨平台的 C++ HTTP/HTTPS 库。
- **JSON 处理**: `nlohmann/json`，用于序列化和反序列化 JSON 数据。
- **断言**: C++ 的 `assert()` 宏，用于在测试失败时立即中止程序。

### 2.2. 执行流程
1. **启动服务器**: 在执行测试前，必须先在 `backend/build` 目录下启动 `os_simulator` 服务器实例。
2. **运行测试**: 在另一个终端中，执行 `api_tests` 可执行文件（或通过 `ctest`）。测试程序会自动连接到 `localhost:8080` 并按顺序执行所有测试模块。
3. **结果输出**: 每个测试函数的执行结果会实时打印到控制台，最终汇总所有测试的通过情况。

### 2.3. 编译与运行

**编译**:
```bash
cd backend/build
cmake ..
make # 或者 cmake --build .
```

**运行**:
```bash
# 终端 1: 启动服务器
./os_simulator

# 终端 2: 运行测试
./api_tests
```

---

## 3. 测试模块详解

### 3.1. 进程管理 (`process_api_test.cpp`)

#### 测试目标
验证进程的完整生命周期管理，包括创建、查询和终止，并测试相关的错误处理逻辑。

#### 覆盖的 API 接口
- `GET /api/v1/processes`: 获取系统中所有进程的列表。
- `POST /api/v1/processes`: 创建一个新进程。
- `DELETE /api/v1/processes/{pid}`: 终止一个指定 PID 的进程。

#### 核心测试用例详解

1.  **`test_get_processes`**:
    - **目的**: 验证获取进程列表接口能否成功返回，并且返回的进程数量是否与预期一致。
    - **步骤**:
        1. 向 `GET /api/v1/processes` 发送请求。
        2. 断言响应状态码为 `200 OK`。
        3. 解析响应体 JSON，断言 `status` 字段为 `"success"`，且 `data` 数组的大小等于 `expected_count`。

2.  **`test_create_process`**:
    - **目的**: 测试进程创建功能，包括成功创建和因内存不足（OOM）而失败的场景。
    - **步骤**:
        1. 构建一个包含 `memory_size` 的 JSON 请求体。
        2. 向 `POST /api/v1/processes` 发送请求。
        3. 根据 `should_succeed` 参数，断言响应状态码为 `201 Created`（成功）或 `400 Bad Request`（失败）。
        4. 如果是成功场景，则解析响应体，验证返回的 PID 是否有效。
    - **代码样例**:
      ```cpp
      void test_create_process(httplib::Client& cli, uint64_t size, bool should_succeed, ProcessID* out_pid = nullptr) {
          json req_body = {{"memory_size", size}};
          auto res = cli.Post("/api/v1/processes", req_body.dump(), "application/json");
          assert(res);
          int expected_status = should_succeed ? 201 : 400;
          assert(res->status == expected_status);

          if (should_succeed) {
              json body = json::parse(res->body);
              assert(body["status"] == "success");
              assert(body["data"]["pid"].is_number());
              if (out_pid) {
                  *out_pid = body["data"]["pid"];
              }
          }
      }
      ```

3.  **`test_terminate_process`**:
    - **目的**: 验证进程终止功能，以及终止不存在进程时的错误处理。
    - **步骤**:
        1. 构建指向特定 PID 的 URL（如 `/api/v1/processes/101`）。
        2. 向该 URL 发送 `DELETE` 请求。
        3. 根据 `should_succeed` 参数，断言响应状态码为 `200 OK`（成功）或 `404 Not Found`（失败）。

### 3.2. 内存管理 (`memory_api_test.cpp`)

#### 测试目标
验证内存状态查询、分配策略切换以及与策略相关的特定字段的正确性。

#### 覆盖的 API 接口
- `GET /api/v1/memory/status`: 获取当前内存的详细状态。
- `PUT /api/v1/memory/strategy`: 切换内存分配策略。

#### 核心测试用例详解

1.  **`test_get_memory_status`**:
    - **目的**: 验证内存状态接口返回的数据完整且正确。
    - **步骤**:
        1. 向 `GET /api/v1/memory/status` 发送请求。
        2. 断言响应状态码为 `200 OK`，且 `status` 字段为 `"success"`。
        3. 验证响应体中包含 `total_memory`, `used_memory`, `allocation_strategy` 等核心字段。
        4. **条件验证**: 根据当前的 `allocation_strategy` 值，进一步验证是否存在对应的特定字段（如 `free_blocks`, `partitions`, `paging`）。

2.  **`test_memory_strategy_api`**:
    - **目的**: 验证内存分配策略的切换功能，并测试无效策略值的处理。
    - **步骤**:
        1. 向 `PUT /api/v1/memory/strategy` 发送请求，将策略切换为 `1` (分区)。
        2. 断言响应状态码为 `200 OK`。
        3. 再次调用 `GET /api/v1/memory/status`，断言返回的策略值确实已变为 `1`。
        4. 重复上述步骤，切换策略为 `2` (分页) 并验证。
        5. 尝试发送一个无效的策略值（如 `5`），并断言响应状态码为 `400 Bad Request`。
        6. 最后恢复默认策略 `0` (连续)。
    - **代码样例**:
      ```cpp
      // 测试设置分区分配策略
      json request_body = {{"strategy", 1}};
      auto res = cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
      assert(res && res->status == 200);
      
      // 验证策略是否真的改变了
      auto status_res = cli.Get("/api/v1/memory/status");
      assert(status_res && status_res->status == 200);
      auto data = json::parse(status_res->body);
      assert(data["data"]["allocation_strategy"] == 1);
      ```

### 3.3. 文件系统 (`filesystem_api_test.cpp`)

#### 测试目标
全面测试文件系统的所有核心操作，包括目录和文件的增删改查，并验证在外存分配策略切换下的行为一致性。

#### 覆盖的 API 接口
- `GET /api/v1/filesystem/status`: 获取文件系统状态。
- `PUT /api/v1/filesystem/config`: 切换外存分配策略。
- `POST /api/v1/filesystem/directory`: 创建目录。
- `GET /api/v1/filesystem/directory/{path}`: 列出目录内容。
- `POST /api/v1/filesystem/file`: 创建文件。
- `GET /api/v1/filesystem/file/{path}`: 读取文件元数据。
- `GET /api/v1/filesystem/file-address?path={path}`: 获取文件存储的物理地址。
- `DELETE /api/v1/filesystem/{path}`: 删除文件或目录。

#### 核心测试用例详解

1.  **`run_strategy_test`**:
    - **这是一个复合测试**，它针对每一种外存分配策略（`CONTIGUOUS`, `LINKED`, `INDEXED`）执行一整套文件系统操作流程。
    - **目的**: 确保在不同分配策略下，文件系统的逻辑表现一致。
    - **流程**:
        1. **设置策略**: 调用 `test_set_allocation_strategy` 切换到指定策略。
        2. **创建**: 创建嵌套目录 (`/test_run_STRATEGY/subdir`) 和多个文件，并验证 API 返回成功。
        3. **验证**: 通过 `test_list_directory` 确认目录内容，通过 `test_read_file` 确认文件元数据，通过 `test_get_file_address` 确认文件地址块已按当前策略分配。
        4. **删除**: 测试删除非空目录失败、删除文件成功、最后递归删除整个测试目录成功。

2.  **地址重新计算测试**:
    - **目的**: 验证当文件已存在时，切换外存分配策略后，文件的物理地址能够被正确地重新计算和查询。
    - **步骤**:
        1. 使用 `CONTIGUOUS` 策略创建一个文件，并查询其地址。
        2. 将策略切换为 `LINKED`，不修改文件，再次查询同一文件的地址，并验证返回的地址结构符合链接分配的格式。
        3. 将策略切换为 `INDEXED`，重复上述验证。

### 3.4. 设备管理 (`device_api_test.cpp`)

#### 测试目标
验证设备的申请、释放、操作和删除等功能，并测试并发和错误情况。

#### 覆盖的 API 接口
- `GET /api/v1/devices`: 获取所有设备列表。
- `POST /api/v1/devices/request`: 请求使用一个设备。
- `POST /api/v1/devices/{id}/release`: 释放一个设备。
- `POST /api/v1/devices/{id}/operation`: 对设备执行一个操作。
- `DELETE /api/v1/devices/{id}`: 删除一个空闲设备。

#### 核心测试用例详解

1.  **设备生命周期测试**:
    - **目的**: 模拟一个进程申请、使用、释放设备的完整流程。
    - **步骤**:
        1. `test_get_devices` 获取一个空闲设备ID。
        2. `test_request_device` 使用一个模拟的 `pid` 申请该设备，断言成功。
        3. 再次调用 `test_request_device` 尝试用另一个 `pid` 申请**同一设备**，断言失败（`400 Bad Request`），验证了设备的互斥使用。
        4. `test_device_operation` 模拟对该设备的一次操作，断言成功。
        5. `test_release_device` 释放该设备，断言成功。
        6. 再次调用 `test_release_device` 尝试释放**已空闲**的设备，断言失败。
        7. `test_delete_device` 删除该空闲设备，断言成功。
        8. 再次调用 `test_delete_device` 尝试删除**已删除**的设备，断言失败。

### 3.5. 调度器 (`scheduler_api_test.cpp`)

#### 测试目标
验证调度器 API，包括就绪队列查询、调度算法切换和单步调度（tick）的正确性。

#### 覆盖的 API 接口
- `GET /api/v1/scheduler/ready_queue`: 获取当前就绪队列中的进程。
- `POST /api/v1/scheduler/tick`: 执行一次调度。
- `PUT /api/v1/scheduler/config`: 配置调度算法和相关参数（如时间片）。
- `GET /api/v1/scheduler/config`: 获取当前调度算法配置。
- `GET /api/v1/scheduler/gantt_chart`: 获取用于绘制甘特图的数据。

#### 核心测试用例详解

1.  **调度tick测试**:
    - **目的**: 验证 `tick` 接口能否正确地从就绪队列中选出一个进程执行。
    - **步骤**:
        1. 首先创建几个进程来填充就绪队列。
        2. 调用 `test_get_ready_queue` 记录当前就绪进程数。
        3. 调用 `test_scheduler_tick`，断言其返回了一个有效的进程对象。
        4. 再次调用 `test_get_ready_queue`，验证就绪队列大小是否符合预期（根据调度算法，可能是减一或不变）。
        5. 清理创建的进程，并再次调用 `tick`，断言其返回 `null`，因为队列已空。

2.  **算法切换测试**:
    - **目的**: 验证调度算法可以被动态切换。
    - **步骤**:
        1. 使用 `PUT /api/v1/scheduler/config` 将算法设置为 `SJF`。
        2. 调用 `GET /api/v1/scheduler/config` 确认算法已更改。
        3. 切换到 `RR` 并设置 `time_slice`，重复验证。

### 3.6. 时钟与中断

#### 测试目标
验证时钟和中断相关的 API，确保可以查询时间、设置定时器和触发中断。

#### 覆盖的 API 接口
- `GET /api/v1/clock/time`: 获取系统当前时间和 tick 数。
- `POST /api/v1/clock/timer`: 创建一个一次性或周期性定时器。
- `GET /api/v1/interrupts/vector_table`: 获取中断向量表。
- `POST /api/v1/interrupts/handler`: 注册中断处理程序。
- `POST /api/v1/interrupts/trigger`: 手动触发一个中断。

#### 核心测试用例详解
- **定时器测试**: 调用 `test_create_timer` 创建一个一次性定时器和一个周期性定时器，并断言成功。
- **中断注册与触发**: 调用 `test_register_handler` 为向量 `32` 和 `33` 注册处理程序，然后尝试为 `32` 重复注册并断言失败。接着，调用 `test_trigger_interrupt` 触发 `32` (成功) 和一个未注册的向量 `40` (失败 `404 Not Found`)。

### 3.7. 进程关系 (`relationship_api_test.cpp`)

#### 测试目标
验证进程间同步关系（当一个进程阻塞，另一个也随之阻塞）的建立和状态传播。

#### 覆盖的 API 接口
- `POST /api/v1/processes/relationship`: 创建两个进程间的关系。
- `GET /api/v1/processes/relationships`: 获取所有关系列表。
- `PUT /api/v1/processes/{pid}/state`: 修改一个进程的状态。

#### 核心测试用例详解
1.  **创建进程**: 创建两个测试进程 A 和 B。
2.  **建立关系**: 调用 `POST /api/v1/processes/relationship` 建立 A 和 B 的 `SYNC` 关系。
3.  **阻塞传播**: 调用 `PUT /api/v1/processes/{pidA}/state` 将 A 的状态设为 `BLOCKED`。
4.  **验证**: 调用 `GET /api/v1/processes` 获取所有进程列表，断言进程 B 的状态也变成了 `BLOCKED`。
5.  **唤醒传播**: 再次调用接口将 A 的状态设为 `READY`，并验证 B 的状态也随之变为 `READY`。

### 3.8. 进程同步 (`sync_api_test.cpp`)

#### 测试目标
验证基于信号量的进程同步机制 API。

#### 覆盖的 API 接口
- `POST /api/v1/sync/semaphore`: 创建一个信号量。
- `POST /api/v1/sync/semaphore/{id}/operation`: 对信号量执行 P/V 操作。

#### 核心测试用例详解
1.  **创建与操作**: `test_create_semaphore` 创建一个初始值为 1 的信号量。然后 `test_semaphore_operation` 先后执行 P 操作（成功）和 V 操作（成功）。
2.  **并发测试**: `test_concurrent_semaphore_operations` 创建多个线程，同时对初始值为 1 的信号量执行 P 操作。等待所有线程结束后，断言只有一个线程成功，验证了信号量的互斥性。

### 3.9. 进程间通信 (IPC) (`ipc_api_test.cpp`)

#### 测试目标
验证基于消息队列的进程间通信 API。

#### 覆盖的 API 接口
- `POST /api/v1/ipc/queue`: 创建一个消息队列。
- `POST /api/v1/ipc/queue/{id}/send`: 向队列发送消息。
- `POST /api/v1/ipc/queue/{id}/receive`: 从队列接收消息。

#### 核心测试用例详解
1.  **收发测试**: `test_create_queue` 创建一个队列，然后 `test_send_message` 发送一条消息，再由 `test_receive_message` 接收，并断言接收到的消息内容与发送的一致。
2.  **溢出测试**: `test_queue_overflow` 向一个容量有限的队列连续发送消息，直到 API 返回 `400 Bad Request`，验证了队列的容量限制。
3.  **并发测试**: `test_concurrent_send_receive` 创建多个发送者线程和接收者线程并发工作，最后断言所有发送的消息都被成功接收。

---

**文档版本**: v3.0 