## **操作系统模拟器 API 文档 V2.1**

**基础URL**: `http://localhost:8080/api/v1`

### **1. 进程管理 (Process Management)**

#### 1.1 获取所有进程信息
获取当前系统中所有进程的列表及其详细状态。

**接口地址**
`GET http://localhost:8080/api/v1/processes`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型          | 描述                                     |
|---------------|---------------|------------------------------------------|
| pid           | integer       | 进程唯一标识符 (Process ID)                |
| name          | string        | 进程名称                                   |
| parent_pid    | integer       | 父进程 ID（-1 表示无父进程）                |
| state         | string        | 进程当前状态 ("NEW", "READY", "RUNNING", "BLOCKED", "TERMINATED") |
| program_counter | integer       | 程序计数器                               |
| cpu_time      | integer       | 进程所需 CPU 时间（毫秒，模拟值）        |
| priority      | integer       | 进程优先级 (数字越小优先级越高)          |
| creation_time | integer(uint64)| 创建时间（毫秒 since epoch）             |
| memory_info   | array (object)| 进程占用的内存块信息                     |
| » base_address| integer(uint64) | 内存块起始地址                           |
| » size        | integer(uint64) | 内存块大小（字节）                       |

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": [
        {
          "pid": 1,
          "name": "process1",
          "parent_pid": -1,
          "state": "RUNNING",
          "program_counter": 1024,
          "cpu_time": 500,
          "priority": 3,
          "creation_time": 1684320000,
          "memory_info": [
            {
              "base_address": 0,
              "size": 4096
            }
          ]
        },
        {
          "pid": 2,
          "name": "process2",
          "parent_pid": 1,
          "state": "READY",
          "program_counter": 0,
          "cpu_time": 0,
          "priority": 5,
          "creation_time": 1684320000,
          "memory_info": [
            {
              "base_address": 4096,
              "size": 8192
            }
          ]
        }
      ]
    }
    ```

#### 1.2 创建新进程
请求操作系统内核创建一个新的进程，并为其分配指定大小的内存。

**接口地址**
`POST http://localhost:8080/api/v1/processes`

**参数描述**
*   **请求参数**

| 参数名        | 类型            | 是否必须 | 描述           |
|---------------|-----------------|----------|----------------|
| memory_size   | integer(uint64) | 是       | 请求的内存大小（字节） |
| cpu_time      | integer         | 否       | 进程预计 CPU 时间（毫秒），默认 10 |
| priority      | integer         | 否       | 进程优先级，默认 5 |

*   **响应参数**
    成功时，响应体为新创建进程的完整信息，结构同 `1.1` 中的单个进程对象。

**请求示例**
```json
{
  "memory_size": 16384,
  "cpu_time": 500,
  "priority": 3
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "message": "Process created successfully.",
      "data": {
        "pid": 3,
        "name": "process3",
        "parent_pid": 1,
        "state": "READY",
        "program_counter": 0,
        "cpu_time": 500,
        "priority": 3,
        "creation_time": 1684320000,
        "memory_info": [
          {
            "base_address": 12288,
            "size": 16384
          }
        ]
      }
    }
    ```
*   失败 (400 Bad Request):
    ```json
    {
      "status": "error",
      "message": "Insufficient memory to create process."
    }
    ```

#### 1.3 终止进程
根据进程ID终止一个指定的进程，并释放其占用的所有资源。

**接口地址**
`DELETE http://localhost:8080/api/v1/processes/{pid}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述                  |
|--------|---------|-----------------------|
| pid    | integer | 要终止的进程的ID      |

*   **响应参数**: 无

**请求示例**
无 (请求路径为 `http://localhost:8080/api/v1/processes/3`)

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "Process 3 terminated successfully."
    }
    ```
*   失败 (404 Not Found):
    ```json
    {
      "status": "error",
      "message": "Process not found."
    }
    ```

#### 1.4 更新进程状态
根据进程ID更新其状态（支持 `NEW / READY / RUNNING / BLOCKED / TERMINATED`）。

**接口地址**
`PUT http://localhost:8080/api/v1/processes/{pid}/state`

**请求参数**
| 参数名 | 类型 | 是否必须 | 描述 |
|--------|------|----------|------|
| state  | string | 是 | 新状态 |

**响应参数**: 返回更新后的进程对象，结构同 `1.1` 中单个进程对象。

**请求示例**
```json
{
  "state": "BLOCKED"
}
```

#### 1.5 创建子进程
与 `fork()` 类似，根据父进程 ID 创建子进程。

**接口地址**
`POST http://localhost:8080/api/v1/processes/{parent_pid}/children`

**请求参数**
与 `1.2` 创建进程相同，但可选参数 `name` 指定子进程名称。

**响应参数**: 新建子进程信息，结构同 `1.1`。

#### 1.6 创建进程关系
建立两个进程之间的同步或互斥关系。

**接口地址**
`POST http://localhost:8080/api/v1/processes/relationship`

**请求参数**
| 参数名 | 类型 | 是否必须 | 描述 |
|--------|------|----------|------|
| pid1 | integer | 是 | 进程A |
| pid2 | integer | 是 | 进程B |
| relation_type | string | 是 | "SYNC" 或 "MUTEX" |

**响应参数**
无固定结构，示例：
```json
{
  "status": "success",
  "message": "Relationship created",
  "data": {
    "pid1": 10,
    "pid2": 11,
    "relation_type": "SYNC"
  }
}
```

> **同步 (SYNC)**: 若其中一个进程进入 `BLOCKED`，另一方也自动进入 `BLOCKED`；解除阻塞时亦会同时恢复到 `READY`。

#### 1.7 获取进程关系列表
返回当前系统中所有已建立的进程关系（同步或互斥）。

**接口地址**
`GET http://localhost:8080/api/v1/processes/relationships`

**参数描述**
*   **请求参数**: 无

*   **响应参数**: 一个数组，每个元素包含：

| 参数名         | 类型    | 描述                       |
|----------------|---------|----------------------------|
| pid1           | integer | 进程 A 的 PID              |
| pid2           | integer | 进程 B 的 PID              |
| relation_type  | string  | 关系类型 ("SYNC" / "MUTEX")|

**请求示例**
无

**响应示例**
```json
{
  "status": "success",
  "data": [
    { "pid1": 10, "pid2": 11, "relation_type": "SYNC" },
    { "pid1": 3,  "pid2": 7,  "relation_type": "MUTEX" }
  ]
}
```

### **2. 调度器 (Scheduler)**

#### 2.0 调度器配置

获取或设置当前调度算法及时间片大小。

**接口地址**
*   查询: `GET  /api/v1/scheduler/config`
*   设置: `PUT  /api/v1/scheduler/config`

**请求参数 (PUT)**
| 参数名      | 类型    | 是否必须 | 描述                                     |
|-------------|---------|----------|------------------------------------------|
| algorithm   | string  | 是       | 调度算法 ("FCFS", "SJF", "PRIORITY", "RR") |
| time_slice  | integer | 否       | 时间片大小（毫秒，仅 RR 有效，默认 1）    |

**响应参数 (GET & PUT)**
| 参数名      | 类型    | 描述                     |
|-------------|---------|--------------------------|
| algorithm   | string  | 当前调度算法             |
| time_slice  | integer | 当前时间片大小           |

#### 2.1 执行一次调度
手动触发一次调度器操作，从就绪队列中选出下一个进程投入运行。

**接口地址**
`POST http://localhost:8080/api/v1/scheduler/tick`

**参数描述**
*   **请求参数**: 无
*   **响应参数**: 响应体为被调度进程的完整信息，结构同 `1.1` 中的单个进程对象。如果就绪队列为空，则 `data` 字段为 `null`。

**请求示例**
无

**响应示例**
*   成功 (找到就绪进程):
    ```json
    {
      "status": "success",
      "data": {
        "pid": 2,
        "name": "process2",
        "parent_pid": 1,
        "state": "RUNNING",
        "program_counter": 0,
        "memory_info": [
          {
            "base_address": 4096,
            "size": 8192
          }
        ]
      }
    }
    ```
*   成功 (就绪队列为空):
    ```json
    {
      "status": "success",
      "data": null,
      "message": "Ready queue is empty, no process to schedule."
    }
    ```

#### 2.2 查看就绪队列
获取当前在就绪队列中等待调度的所有进程。

**接口地址**
`GET http://localhost:8080/api/v1/scheduler/ready_queue`

**参数描述**
*   **请求参数**: 无
*   **响应参数**: 响应体为一个进程对象数组，结构同 `1.1`。

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": [
        {
          "pid": 4,
          "name": "process4",
          "parent_pid": 2,
          "state": "READY",
          "program_counter": 0,
          "memory_info": [
            {
              "base_address": 28672,
              "size": 1024
            }
          ]
        }
      ]
    }
    ```

#### 2.3 生成甘特图数据
根据当前调度算法和进程队列，返回一张甘特图表。

**接口地址**
`GET http://localhost:8080/api/v1/scheduler/gantt_chart`

**响应参数**
返回按调度顺序排列的数组，每个元素包含:

| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| pid    | integer | 进程ID         |
| start  | integer | 开始时间 (ms)  |
| end    | integer | 结束时间 (ms)  |

### **3. 内存管理 (Memory Management)**
#### 3.1 获取内存状态
获取当前整个系统的内存使用详情。

**接口地址**
`GET http://localhost:8080/api/v1/memory/status`

**参数描述**
*   **请求参数**: 无
*   **响应参数**
| 参数名               | 类型            | 描述                                   |
|---------------------|-----------------|----------------------------------------|
| total_memory        | integer(uint64) | 系统总内存大小（字节）                 |
| used_memory         | integer(uint64) | 已用内存大小（字节）                   |
| allocation_strategy | integer         | 当前内存分配策略 (0=连续, 1=分区, 2=分页) |
| free_blocks         | array (object)  | 空闲内存块列表（仅连续分配时返回）      |
| » base_address      | integer(uint64) | 空闲块起始地址                         |
| » size              | integer(uint64) | 空闲块大小（字节）                     |
| partitions          | array (object)  | 分区信息列表（分区分配时返回）         |
| » base_address      | integer(uint64) | 分区起始地址                           |
| » size              | integer(uint64) | 分区大小（字节）                       |
| » is_free           | boolean         | 分区是否空闲                           |
| » owner_pid         | integer         | 分区拥有者进程ID（-1表示空闲）         |
| paging              | object          | 分页信息（分页分配时返回）             |
| » total_pages       | integer         | 页框总数                               |
| » used_pages        | integer         | 已使用页框数                           |
| » free_pages        | integer         | 空闲页框数                             |

**请求示例**
无

**响应示例**
*   成功（连续分配策略）：
    ```json
    {
      "status": "success",
      "data": {
        "total_memory": 4294967296,
        "used_memory": 104857600,
        "allocation_strategy": 0,
        "free_blocks": [
          {
            "base_address": 104857600,
            "size": 4190109696
          }
        ]
      }
    }
    ```
*   成功（分区分配策略）：
    ```json
    {
      "status": "success",
      "data": {
        "total_memory": 4294967296,
        "used_memory": 1073741824,
        "allocation_strategy": 1,
        "partitions": [
          {
            "base_address": 0,
            "size": 262144,
            "is_free": false,
            "owner_pid": 1
          },
          {
            "base_address": 262144,
            "size": 262144,
            "is_free": true,
            "owner_pid": -1
          }
        ]
      }
    }
    ```

#### 3.2 设置内存分配策略
更改系统当前使用的内存分配策略。

**接口地址**
`PUT http://localhost:8080/api/v1/memory/strategy`

**参数描述**
*   **请求参数**

| 参数名    | 类型    | 是否必须 | 描述                                           |
|-----------|---------|----------|------------------------------------------------|
| strategy  | integer | 是       | 内存分配策略 (0=连续分配, 1=分区分配, 2=分页分配) |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| old_strategy  | integer | 原分配策略     |
| new_strategy  | integer | 新分配策略     |

**请求示例**
```json
{
  "strategy": 1
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "Memory allocation strategy updated successfully.",
      "data": {
        "old_strategy": 0,
        "new_strategy": 1
      }
    }
    ```
*   失败 (400 Bad Request):
    ```json
    {
      "status": "error",
      "message": "Invalid strategy value. Must be 0(CONTINUOUS), 1(PARTITIONED), or 2(PAGED)."
    }
    ```
### **4. 文件系统 (File System)**

#### 4.1 获取文件系统状态
获取当前文件系统的使用情况和配置。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/status`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名              | 类型            | 描述                                     |
|---------------------|-----------------|------------------------------------------|
| total_space         | integer(uint64) | 总空间 (64 GB)                           |
| used_space          | integer(uint64) | 已用空间（字节）                         |
| free_space          | integer(uint64) | 空闲空间（字节）                         |
| total_files         | integer         | 文件总数                                 |
| total_dirs          | integer         | 目录总数                                 |
| allocation_method   | string          | 当前外存分配策略 ("CONTIGUOUS", "LINKED", "INDEXED") |

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": {
        "total_space": 68719476736,
        "used_space": 536870912,
        "free_space": 68182605824,
        "total_files": 10,
        "total_dirs": 7,
        "allocation_method": "INDEXED"
      }
    }
    ```

#### 4.2 更改外存分配策略
更改文件系统当前使用的外存分配策略。

**接口地址**
`PUT http://localhost:8080/api/v1/filesystem/config`

**参数描述**
*   **请求参数**

| 参数名              | 类型    | 是否必须 | 描述                                     |
|---------------------|---------|----------|------------------------------------------|
| allocation_method   | string  | 是       | 新的分配策略 ("CONTIGUOUS", "LINKED", "INDEXED") |

*   **响应参数**: 无

**请求示例**
```json
{
  "allocation_method": "CONTIGUOUS"
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "Allocation strategy updated to CONTIGUOUS"
    }
    ```

#### 4.3 创建目录
在指定路径创建新目录。

**接口地址**
`POST http://localhost:8080/api/v1/filesystem/directory`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| path          | string  | 是       | 目录路径       |
| permissions   | integer | 否       | 目录权限（八进制） |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| path          | string  | 创建的目录路径 |
| permissions   | integer | 目录权限       |

**请求示例**
```json
{
  "path": "/documents/projects",
  "permissions": 755
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "data": {
        "path": "/documents/projects",
        "permissions": 755
      }
    }
    ```

#### 4.4 创建文件
在指定路径创建新文件，并指定其模拟大小。

**接口地址**
`POST http://localhost:8080/api/v1/filesystem/file`

**参数描述**
*   **请求参数**

| 参数名           | 类型            | 是否必须 | 描述                         |
|------------------|-----------------|----------|------------------------------|
| path             | string          | 是       | 文件路径                     |
| simulated_size   | integer(uint64) | 是       | 文件的模拟大小（字节）       |
| permissions      | integer         | 否       | 文件权限（八进制, 默认 644） |

*   **响应参数**

| 参数名           | 类型            | 描述               |
|------------------|-----------------|--------------------|
| path             | string          | 创建的文件路径     |
| simulated_size   | integer(uint64) | 文件的模拟大小     |
| permissions      | integer         | 文件权限           |

**请求示例**
```json
{
  "path": "/home/user/new_large_file.dat",
  "simulated_size": 1048576,
  "permissions": 644
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "data": {
        "path": "/home/user/new_large_file.dat",
        "simulated_size": 1048576,
        "permissions": 644
      }
    }
    ```

#### 4.5 读取文件
读取指定文件的元数据和模拟内容。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/file/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 文件路径 (URL编码) |

*   **请求参数**: 无
*   **响应参数**

| 参数名           | 类型            | 描述                         |
|------------------|-----------------|------------------------------|
| path             | string          | 文件路径                     |
| content          | string          | 文件模拟内容（占位符）       |
| simulated_size   | integer(uint64) | 文件模拟大小（字节）         |
| permissions      | integer         | 文件权限                     |
| created_at       | string          | 创建时间 (ISO 8601)          |
| modified_at      | string          | 最后修改时间 (ISO 8601)      |

**请求示例**
`GET http://localhost:8080/api/v1/filesystem/file/home/user/report.docx`

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "path": "/home/user/report.docx",
        "content": "Simulated file content for: /home/user/report.docx\nSimulated Size: 2097152 bytes\n",
        "simulated_size": 2097152,
        "permissions": 644,
        "created_at": "2024-05-22T10:30:00Z",
        "modified_at": "2024-05-22T10:30:00Z"
      }
    }
    ```

#### 4.6 查询文件首地址
查询指定文件在不同分配策略下的逻辑首地址。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/file-address`

**参数描述**
*   **URL查询参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 文件路径       |

*   **响应参数**

| 参数名                 | 类型    | 描述                               |
|------------------------|---------|------------------------------------|
| path                   | string  | 查询的文件路径                     |
| addresses              | object  | 不同策略下的首地址                 |
| » contiguous           | integer | 连续分配下的起始块号 (如果适用)    |
| » linked               | integer | 链接分配下的起始块号 (如果适用)    |
| » indexed              | integer | 索引分配下的索引块号 (如果适用)    |

**请求示例**
`GET http://localhost:8080/api/v1/filesystem/file-address?path=/etc/config.json`

**响应示例**
*   成功 (200 OK):
    ```json
    {
        "status": "success",
        "data": {
            "path": "/etc/config.json",
            "addresses": {
                "contiguous": null,
                "linked": null,
                "indexed": 15
            }
        }
    }
    ```
    *注：对于一个已创建的文件，只有一个地址字段会有值，具体取决于文件创建时所采用的分配策略。*

#### 4.7 删除文件或目录
删除指定的文件或目录 (目录会递归删除)。

**接口地址**
`DELETE http://localhost:8080/api/v1/filesystem/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 文件或目录路径 |

*   **请求参数**: 无
*   **响应参数**: 无

**请求示例**
无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "File or directory deleted successfully"
    }
    ```

#### 4.8 列出目录内容
获取指定目录下的所有文件和子目录。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/directory/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述               |
|--------|---------|--------------------|
| path   | string  | 目录路径 (URL编码) |

*   **请求参数**: 无
*   **响应参数**: 一个包含文件和目录对象的数组。

| 参数名        | 类型            | 描述                         |
|---------------|-----------------|------------------------------|
| name          | string          | 文件或目录名                 |
| type          | string          | "file" 或 "directory"        |
| size          | integer(uint64) | 对于文件，为模拟大小（字节） |
| permissions   | integer         | 权限                         |
| created_at    | string          | 创建时间 (ISO 8601)          |
| modified_at   | string          | 最后修改时间 (ISO 8601)      |

**请求示例**
`GET http://localhost:8080/api/v1/filesystem/directory/home/user`

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": [
        {
          "name": "documents",
          "type": "directory",
          "size": 0,
          "permissions": 755,
          "created_at": "2024-05-22T10:29:00Z",
          "modified_at": "2024-05-22T10:29:00Z"
        },
        {
          "name": "new_large_file.dat",
          "type": "file",
          "size": 1048576,
          "permissions": 644,
          "created_at": "2024-05-22T10:35:00Z",
          "modified_at": "2024-05-22T10:35:00Z"
        }
      ]
    }
    ```

### **5. 设备管理 (Device Management)**

#### 5.1 获取设备列表
获取系统中所有可用设备的信息。

**接口地址**
`GET http://localhost:8080/api/v1/devices`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| device_id     | integer | 设备ID         |
| name          | string  | 设备名称       |
| type          | string  | 设备类型       |
| status        | string  | 设备状态 ("IDLE", "BUSY", "ERROR") |
| current_user  | integer | 当前使用设备的进程ID（如果空闲则为null） |

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": [
        {
          "device_id": 1,
          "name": "printer1",
          "type": "PRINTER",
          "status": "IDLE",
          "current_user": null
        },
        {
          "device_id": 2,
          "name": "disk1",
          "type": "DISK",
          "status": "BUSY",
          "current_user": 123
        }
      ]
    }
    ```

#### 5.2 申请设备（更新）
请求分配指定 ID 的设备给某个进程。

**接口地址**
`POST http://localhost:8080/api/v1/devices/request`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| device_id     | integer | 是       | 要申请的设备 ID |
| process_id    | integer | 是       | 发起申请的进程 ID |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| device_id     | integer | 设备 ID        |
| name          | string  | 设备名称       |
| type          | string  | 设备类型       |
| status        | string  | 设备状态 (BUSY)|
| current_user  | integer | 当前占用该设备的进程 ID |

**请求示例**
```json
{
  "device_id": 1,
  "process_id": 101
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "device_id": 1,
        "name": "printer1",
        "type": "PRINTER",
        "status": "BUSY",
        "current_user": 101
      }
    }
    ```
*   失败 (400 Bad Request):
    ```json
    {
      "status": "error",
      "message": "Device is busy or not found"
    }
    ```

#### 5.3 释放设备（更新）
释放一个正在使用的设备。

**接口地址**
`POST http://localhost:8080/api/v1/devices/{device_id}/release`

**参数描述**
*   **URL 参数**
| 参数名    | 类型    | 描述           |
|-----------|---------|----------------|
| device_id | integer | 设备 ID        |

*   **请求参数**
| 参数名     | 类型    | 是否必须 | 描述                 |
|------------|---------|----------|----------------------|
| process_id | integer | 是       | 释放设备的进程 ID     |

*   **响应参数**: 无

**请求示例**
```json
{
  "process_id": 101
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "Device released successfully"
    }
    ```
*   失败 (400 Bad Request):
    ```json
    {
      "status": "error",
      "message": "Device is not in use, not owned by this process, or not found"
    }
    ```

#### 5.4 删除设备（新增）
删除一个空闲设备。

**接口地址**
`DELETE http://localhost:8080/api/v1/devices/{device_id}`

**参数描述**
*   **URL 参数**
| 参数名    | 类型    | 描述           |
|-----------|---------|----------------|
| device_id | integer | 设备 ID        |

*   **响应参数**: 无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "message": "Device deleted successfully"
    }
    ```
*   失败 (400 Bad Request / 404 Not Found):
    ```json
    {
      "status": "error",
      "message": "Device not found or is busy"
    }
    ```

#### 5.5 设备操作
对设备执行特定操作。

**接口地址**
`POST http://localhost:8080/api/v1/devices/{device_id}/operation`

**参数描述**
*   **URL参数**
| 参数名    | 类型    | 描述           |
|-----------|---------|----------------|
| device_id | integer | 设备ID         |

*   **请求参数**

| 参数名    | 类型    | 是否必须 | 描述           |
|-----------|---------|----------|----------------|
| operation | string  | 是       | 操作类型       |
| params    | object  | 否       | 操作参数       |

*   **响应参数**

| 参数名    | 类型    | 描述           |
|-----------|---------|----------------|
| success   | boolean | 操作是否成功   |
| result    | object  | 操作结果       |

**请求示例**
```json
{
  "operation": "PRINT",
  "params": {
    "file_path": "/documents/test.txt",
    "copies": 2
  }
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "success": true,
        "result": {
          "job_id": "PRINT_123",
          "estimated_time": 30
        }
      }
    }
    ```

### **7. 中断处理 (Interrupt Handling)**

#### 7.1 注册中断处理程序
注册一个中断处理程序到指定的中断向量。

**接口地址**
`POST http://localhost:8080/api/v1/interrupts/handler`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| vector        | integer | 是       | 中断向量号     |
| handler_type  | string  | 是       | 处理程序类型 ("TIMER", "IO", "SYSCALL") |
| priority      | integer | 否       | 中断优先级     |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| vector        | integer | 中断向量号     |
| handler_type  | string  | 处理程序类型   |
| priority      | integer | 中断优先级     |

**请求示例**
```json
{
  "vector": 32,
  "handler_type": "TIMER",
  "priority": 1
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "data": {
        "vector": 32,
        "handler_type": "TIMER",
        "priority": 1
      }
    }
    ```

#### 7.2 触发中断
手动触发一个中断。

**接口地址**
`POST http://localhost:8080/api/v1/interrupts/trigger`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| vector        | integer | 是       | 中断向量号     |
| data          | object  | 否       | 中断数据       |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| success       | boolean | 是否成功触发   |
| handler_result| object  | 处理程序返回结果 |

**请求示例**
```json
{
  "vector": 32,
  "data": {
    "tick_count": 1000
  }
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "success": true,
        "handler_result": {
          "processed": true,
          "next_tick": 2000
        }
      }
    }
    ```

#### 7.3 获取中断向量表
获取当前系统的中断向量表信息。

**接口地址**
`GET http://localhost:8080/api/v1/interrupts/vector_table`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| vectors       | array   | 中断向量表     |
| » vector      | integer | 中断向量号     |
| » handler_type| string  | 处理程序类型   |
| » priority    | integer | 中断优先级     |
| » enabled     | boolean | 是否启用       |

**请求示例**
无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "vectors": [
          {
            "vector": 32,
            "handler_type": "TIMER",
            "priority": 1,
            "enabled": true
          },
          {
            "vector": 33,
            "handler_type": "IO",
            "priority": 2,
            "enabled": true
          }
        ]
      }
    }
    ```

### **8. 时钟管理 (Clock Management)**

#### 8.1 设置时钟中断间隔
设置系统时钟中断的时间间隔。

**接口地址**
`POST http://localhost:8080/api/v1/clock/interval`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| interval      | integer | 是       | 中断间隔（毫秒）|

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| old_interval  | integer | 原中断间隔     |
| new_interval  | integer | 新中断间隔     |

**请求示例**
```json
{
  "interval": 100
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "old_interval": 50,
        "new_interval": 100
      }
    }
    ```

#### 8.2 获取系统时间
获取当前系统时间。

**接口地址**
`GET http://localhost:8080/api/v1/clock/time`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| ticks         | integer | 系统时钟滴答数 |
| uptime        | integer | 系统运行时间（毫秒）|
| real_time     | string  | 实际时间（ISO 8601格式）|

**请求示例**
无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "ticks": 1000000,
        "uptime": 50000,
        "real_time": "2024-03-15T10:30:00Z"
      }
    }
    ```

#### 8.3 设置定时器
设置一个定时器，在指定时间后触发中断。

**接口地址**
`POST http://localhost:8080/api/v1/clock/timer`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| delay         | integer | 是       | 延迟时间（毫秒）|
| repeat        | boolean | 否       | 是否重复触发   |
| interval      | integer | 否       | 重复间隔（毫秒）|

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| timer_id      | integer | 定时器ID       |
| trigger_time  | string  | 触发时间       |

**请求示例**
```json
{
  "delay": 1000,
  "repeat": true,
  "interval": 500
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "data": {
        "timer_id": 1,
        "trigger_time": "2024-03-15T10:30:01Z"
      }
    }
    ```