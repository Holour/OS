# 操作系统模拟器 API 集成测试文档

## 概述

本文档详细说明了操作系统模拟器后端 API 的集成测试架构、测试内容和执行方式。集成测试通过 HTTP 客户端调用真实的 API 接口，验证各模块间的协作和端到端功能。

## 测试架构

### 测试框架
- **HTTP 客户端**: 使用 `httplib` 库进行 HTTP 请求
- **JSON 解析**: 使用 `nlohmann/json` 处理请求和响应
- **断言验证**: 使用 `assert` 宏验证预期结果
- **测试组织**: 按功能模块分组，每个模块独立测试文件

### 执行流程
1. **服务器启动**: 测试前需要启动 `os_simulator` 服务器
2. **连接等待**: 测试程序等待服务器完全初始化（最多30秒）
3. **模块测试**: 按顺序执行各功能模块的测试
4. **结果汇总**: 输出测试结果和统计信息

## 测试模块详解

### 1. 进程管理 API 测试 (`process_api_test.cpp`)

#### 测试内容
- **获取进程列表**: 验证 `GET /api/v1/processes` 返回正确格式
- **创建进程**: 测试 `POST /api/v1/processes` 的成功和失败场景
- **终止进程**: 验证 `DELETE /api/v1/processes/{pid}` 的正确性
- **错误处理**: 测试内存不足、进程不存在等异常情况

#### 测试样例

```cpp
// 创建进程测试
void test_create_process(httplib::Client& cli, uint64_t size, bool should_succeed, ProcessID* out_pid) {
    json req_body = {{"memory_size", size}};
    auto res = cli.Post("/api/v1/processes", req_body.dump(), "application/json");
    
    int expected_status = should_succeed ? 201 : 400;
    assert(res->status == expected_status);
    
    if (should_succeed) {
        json body = json::parse(res->body);
        assert(body["status"] == "success");
        assert(body["data"]["pid"].is_number());
        if (out_pid) *out_pid = body["data"]["pid"];
    }
}
```

#### 测试场景
1. **正常创建**: 分配 1024 字节内存的进程
2. **内存不足**: 请求超过系统总内存的进程
3. **进程计数**: 验证创建后进程数量变化
4. **终止验证**: 确认进程正确从系统中移除

### 2. 内存管理 API 测试 (`memory_api_test.cpp`)

#### 测试内容
- **内存状态查询**: 验证 `GET /api/v1/memory/status` 返回完整信息
- **分配策略切换**: 测试 `PUT /api/v1/memory/strategy` 的三种策略
- **策略验证**: 确认策略切换后系统状态正确更新
- **字段完整性**: 根据不同策略验证特定字段存在

#### 测试样例

```cpp
// 内存策略切换测试
void test_memory_strategy_api() {
    httplib::Client cli("localhost", 8080);
    
    // 测试设置分区分配策略
    json request_body = {{"strategy", 1}};
    auto res = cli.Put("/api/v1/memory/strategy", request_body.dump(), "application/json");
    assert(res->status == 200);
    
    // 验证策略是否真的改变了
    auto status_res = cli.Get("/api/v1/memory/status");
    json data = json::parse(status_res->body);
    assert(data["data"]["allocation_strategy"] == 1);
}
```

#### 验证要点
- **连续分配** (strategy=0): 验证 `free_blocks` 字段
- **分区分配** (strategy=1): 验证 `partitions` 字段
- **分页分配** (strategy=2): 验证 `paging` 字段包含页框统计

### 3. 调度器 API 测试 (`scheduler_api_test.cpp`)

#### 测试内容
- **就绪队列查询**: 验证 `GET /api/v1/scheduler/ready_queue`
- **调度执行**: 测试 `POST /api/v1/scheduler/tick` 的调度逻辑
- **算法配置**: 验证 `PUT /api/v1/scheduler/config` 支持的算法
- **甘特图生成**: 测试 `GET /api/v1/scheduler/gantt_chart` 数据

#### 测试样例

```cpp
// 调度器配置测试
void test_scheduler_config() {
    auto putBody = json{{"algorithm","SJF"}}.dump();
    auto putRes = cli.Put("/api/v1/scheduler/config", putBody, "application/json");
    assert(putRes->status == 200);

    // 验证当前算法
    auto cfgRes = cli.Get("/api/v1/scheduler/config");
    assert(json::parse(cfgRes->body)["data"]["algorithm"] == "SJF");
}
```

#### 支持的调度算法
- **FCFS**: 先来先服务
- **SJF**: 短作业优先
- **PRIORITY**: 优先级调度
- **RR**: 时间片轮转（可配置时间片大小）

### 4. 文件系统 API 测试 (`filesystem_api_test.cpp`)

#### 测试内容
- **文件系统状态**: 验证 `GET /api/v1/filesystem/status`
- **目录操作**: 测试目录创建、列表、删除
- **文件操作**: 测试文件创建、读取、删除
- **分配策略**: 验证三种外存分配策略的切换
- **地址查询**: 测试文件地址查询功能

#### 测试样例

```cpp
// 文件系统策略测试
void run_strategy_test(httplib::Client& cli, const std::string& strategy_name) {
    test_set_allocation_strategy(cli, strategy_name);
    
    const std::string TEST_DIR = "/test_run_" + strategy_name;
    const std::string FILE_1 = TEST_DIR + "/file1.txt";
    
    // 创建目录和文件
    test_create_directory(cli, TEST_DIR, 201);
    test_create_file(cli, FILE_1, 5000, 201);
    
    // 验证地址分配
    test_get_file_address(cli, FILE_1, strategy_name);
    
    // 清理
    test_delete(cli, FILE_1, 200);
    test_delete(cli, TEST_DIR, 200);
}
```

#### 支持的分配策略
- **CONTIGUOUS**: 连续分配
- **LINKED**: 链接分配  
- **INDEXED**: 索引分配

### 5. 设备管理 API 测试 (`device_api_test.cpp`)

#### 测试内容
- **设备列表**: 验证 `GET /api/v1/devices` 返回设备信息
- **设备申请**: 测试 `POST /api/v1/devices/request` 的分配逻辑
- **设备释放**: 验证 `POST /api/v1/devices/{id}/release` 功能
- **设备操作**: 测试设备特定操作接口
- **设备删除**: 验证空闲设备的删除功能

#### 测试样例

```cpp
// 设备申请和释放测试
void test_device_lifecycle() {
    int device_id = 0;  // 假设设备ID为0
    int process_id = 100;
    
    // 申请设备
    test_request_device(cli, device_id, process_id, 200);
    
    // 再次申请同一设备（应失败）
    test_request_device(cli, device_id, process_id + 1, 400);
    
    // 执行设备操作
    test_device_operation(cli, device_id, "PRINT", 200);
    
    // 释放设备
    test_release_device(cli, device_id, process_id, 200);
}
```

### 6. 中断管理 API 测试 (`interrupt_api_test.cpp`)

#### 测试内容
- **中断向量表**: 验证 `GET /api/v1/interrupts/vector_table`
- **处理程序注册**: 测试 `POST /api/v1/interrupts/handler`
- **中断触发**: 验证 `POST /api/v1/interrupts/trigger` 功能

### 7. 时钟管理 API 测试 (`clock_api_test.cpp`)

#### 测试内容
- **系统时间**: 验证 `GET /api/v1/clock/time` 返回格式
- **时钟间隔**: 测试 `POST /api/v1/clock/interval` 配置
- **定时器**: 验证 `POST /api/v1/clock/timer` 功能

### 8. 进程关系 API 测试 (`relationship_api_test.cpp`)

#### 测试内容
- **关系创建**: 测试进程间同步和互斥关系
- **关系查询**: 验证关系列表接口
- **状态传播**: 测试关系对进程状态的影响

## 执行方式

### 编译测试
```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 运行测试

#### 1. 启动服务器
```bash
./os_simulator
```

#### 2. 执行集成测试
```bash
# 在另一个终端窗口
./api_tests
```

#### 3. 使用 CTest
```bash
ctest --verbose
```

## 测试输出示例

```
--- Running Process Management API Tests ---
Initial process count: 15
Test GET /api/v1/processes (count: 15): PASSED
Test POST /api/v1/processes (size: 1024, success: true): PASSED
Test GET /api/v1/processes (count: 16): PASSED
Test DELETE /api/v1/processes/16 (success: true): PASSED
--- All Process Management API tests passed! ---

=== Memory Management API Tests ===
✓ Memory Status API: PASSED
  Total Memory: 4096 MB
  Used Memory: 102 MB
  Allocation Strategy: 0
✓ Memory Strategy Change API: PASSED
--- Memory API tests completed ---

========================================
  ALL API TESTS PASSED SUCCESSFULLY!  
========================================
```

## 错误处理验证

每个模块都包含错误处理测试：

### 常见错误场景
- **400 Bad Request**: 无效参数、资源冲突
- **404 Not Found**: 资源不存在
- **409 Conflict**: 资源已存在
- **500 Internal Server Error**: 服务器内部错误

### 验证要点
- 错误状态码正确返回
- 错误消息格式符合 API 规范
- 系统状态在错误后保持一致

## 性能考虑

### 测试优化
- **连接复用**: 使用持久 HTTP 连接
- **超时设置**: 合理的连接和请求超时
- **并发控制**: 避免测试间的资源竞争

### 资源清理
- 每个测试模块负责清理自己创建的资源
- 使用 RAII 模式确保异常安全
- 测试失败时的回滚机制

## 扩展指南

### 添加新测试
1. 在 `tests/` 目录创建新的测试文件
2. 实现测试函数并在 `api_tests.cpp` 中注册
3. 更新 `CMakeLists.txt` 包含新的源文件
4. 更新本文档说明新增的测试内容

### 测试最佳实践
- **独立性**: 每个测试应该独立运行
- **可重复**: 测试结果应该可重现
- **清晰性**: 测试名称和输出应该清楚表达意图
- **完整性**: 覆盖正常流程和异常情况

---

**文档版本**: v2.1  
**最后更新**: 2025年6月  
**维护者**: OS 模拟器开发团队 