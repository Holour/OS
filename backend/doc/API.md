## **操作系统模拟器 API 文档 V1.0**

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
| state         | string        | 进程当前状态 ("NEW", "READY", "RUNNING", "BLOCKED", "TERMINATED") |
| program_counter | integer       | 程序计数器                               |
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
          "state": "RUNNING",
          "program_counter": 1024,
          "memory_info": [
            {
              "base_address": 0,
              "size": 4096
            }
          ]
        },
        {
          "pid": 2,
          "state": "READY",
          "program_counter": 0,
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

*   **响应参数**
    成功时，响应体为新创建进程的完整信息，结构同 `1.1` 中的单个进程对象。

**请求示例**
```json
{
  "memory_size": 16384
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
        "state": "READY",
        "program_counter": 0,
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

### **2. 调度器 (Scheduler)**

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
### **3. 内存管理 (Memory Management)**
#### 3.1 获取内存状态
获取当前整个系统的内存使用详情。

**接口地址**
`GET http://localhost:8080/api/v1/memory/status`

**参数描述**
*   **请求参数**: 无
*   **响应参数**
| 参数名        | 类型            | 描述                       |
|---------------|-----------------|----------------------------|
| total_memory  | integer(uint64) | 系统总内存大小（字节）     |
| used_memory   | integer(uint64) | 已用内存大小（字节）       |
| free_blocks   | array (object)  | 空闲内存块列表             |
| » base_address| integer(uint64) | 空闲块起始地址             |
| » size        | integer(uint64) | 空闲块大小（字节）         |

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": {
        "total_memory": 268435456,
        "used_memory": 10514432,
        "free_blocks": [
          {
            "base_address": 10514432,
            "size": 257921024
          }
        ]
      }
    }
    ```
### **4. 文件系统 (File System)**

#### 4.1 获取文件系统状态
获取当前文件系统的使用情况。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/status`

**参数描述**
*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| total_space   | integer | 总空间（字节） |
| used_space    | integer | 已用空间（字节）|
| free_space    | integer | 空闲空间（字节）|
| total_files   | integer | 文件总数       |
| total_dirs    | integer | 目录总数       |

**请求示例**
无

**响应示例**
*   成功：
    ```json
    {
      "status": "success",
      "data": {
        "total_space": 1073741824,
        "used_space": 536870912,
        "free_space": 536870912,
        "total_files": 100,
        "total_dirs": 20
      }
    }
    ```

#### 4.2 创建目录
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

#### 4.3 创建文件
在指定路径创建新文件。

**接口地址**
`POST http://localhost:8080/api/v1/filesystem/file`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| path          | string  | 是       | 文件路径       |
| content       | string  | 否       | 文件内容       |
| permissions   | integer | 否       | 文件权限（八进制） |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| path          | string  | 创建的文件路径 |
| size          | integer | 文件大小（字节）|
| permissions   | integer | 文件权限       |

**请求示例**
```json
{
  "path": "/documents/projects/readme.txt",
  "content": "# Project Documentation",
  "permissions": 644
}
```

**响应示例**
*   成功 (201 Created):
    ```json
    {
      "status": "success",
      "data": {
        "path": "/documents/projects/readme.txt",
        "size": 21,
        "permissions": 644
      }
    }
    ```

#### 4.4 读取文件
读取指定文件的内容。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/file/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 文件路径       |

*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| content       | string  | 文件内容       |
| size          | integer | 文件大小（字节）|
| permissions   | integer | 文件权限       |
| last_modified | string  | 最后修改时间   |

**请求示例**
无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "content": "# Project Documentation",
        "size": 21,
        "permissions": 644,
        "last_modified": "2024-03-15T10:30:00Z"
      }
    }
    ```

#### 4.5 写入文件
向指定文件写入内容。

**接口地址**
`PUT http://localhost:8080/api/v1/filesystem/file/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 文件路径       |

*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| content       | string  | 是       | 文件内容       |
| append        | boolean | 否       | 是否追加内容   |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| size          | integer | 文件大小（字节）|
| last_modified | string  | 最后修改时间   |

**请求示例**
```json
{
  "content": "Updated content",
  "append": false
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "size": 15,
        "last_modified": "2024-03-15T10:35:00Z"
      }
    }
    ```

#### 4.6 删除文件或目录
删除指定的文件或目录。

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

#### 4.7 列出目录内容
获取指定目录下的所有文件和子目录。

**接口地址**
`GET http://localhost:8080/api/v1/filesystem/directory/{path}`

**参数描述**
*   **URL参数**
| 参数名 | 类型    | 描述           |
|--------|---------|----------------|
| path   | string  | 目录路径       |

*   **请求参数**: 无
*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| files         | array   | 文件列表       |
| directories   | array   | 子目录列表     |

**请求示例**
无

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "files": [
          {
            "name": "readme.txt",
            "size": 21,
            "permissions": 644,
            "last_modified": "2024-03-15T10:30:00Z"
          }
        ],
        "directories": [
          {
            "name": "src",
            "permissions": 755,
            "last_modified": "2024-03-15T10:30:00Z"
          }
        ]
      }
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

#### 5.2 申请设备
请求分配一个指定类型的设备。

**接口地址**
`POST http://localhost:8080/api/v1/devices/request`

**参数描述**
*   **请求参数**

| 参数名        | 类型    | 是否必须 | 描述           |
|---------------|---------|----------|----------------|
| device_type   | string  | 是       | 设备类型       |
| timeout       | integer | 否       | 等待超时时间（毫秒） |

*   **响应参数**

| 参数名        | 类型    | 描述           |
|---------------|---------|----------------|
| device_id     | integer | 分配的设备ID   |
| name          | string  | 设备名称       |

**请求示例**
```json
{
  "device_type": "PRINTER",
  "timeout": 5000
}
```

**响应示例**
*   成功 (200 OK):
    ```json
    {
      "status": "success",
      "data": {
        "device_id": 1,
        "name": "printer1"
      }
    }
    ```
*   失败 (400 Bad Request):
    ```json
    {
      "status": "error",
      "message": "No available device of type PRINTER"
    }
    ```

#### 5.3 释放设备
释放一个正在使用的设备。

**接口地址**
`POST http://localhost:8080/api/v1/devices/{device_id}/release`

**参数描述**
*   **URL参数**
| 参数名    | 类型    | 描述           |
|-----------|---------|----------------|
| device_id | integer | 设备ID         |

*   **请求参数**: 无
*   **响应参数**: 无

**请求示例**
无

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
      "message": "Device is not in use"
    }
    ```

#### 5.4 设备操作
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