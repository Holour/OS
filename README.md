# 操作系统模拟器 (OS Simulator)

本项目是一个功能完备的操作系统模拟器，通过C++后端实现核心OS功能，并由一个Vue.js驱动的、类似Windows桌面的图形用户界面进行控制和展示。

## ✨ 核心特色

- **前后端分离**: C++后端专注于模拟核心逻辑，Vue.js前端负责美观、交互式的用户界面。
- **模块化后端**: 后端逻辑高度模块化，涵盖进程、内存、文件、设备、时钟、中断和调度等核心功能。
- **图形化界面**: 提供了类似Windows的桌面环境，包括开始菜单、任务栏、窗口管理和实时系统监控。
- **功能丰富的应用**: 内置文件管理器、进程管理器、设备管理器、终端等多种应用程序，以可视化和交互的方式操作底层OS功能。
- **API驱动**: 通过一套清晰的RESTful API连接前后端，便于扩展和测试。

---

## 🏛️ 项目结构

```
.
├── backend/                # C++后端 (OS核心逻辑)
│   ├── include/            # 各模块头文件 (*.h)
│   ├── src/                # 各模块实现文件 (*.cpp)
│   └── ...                 # 其他 (CMake, tests, doc)
└── frontend/               # Vue.js前端 (GUI)
    ├── src/
    │   ├── components/
    │   │   ├── apps/       # 各应用程序的Vue组件
    │   │   │   ├── ProcessManager.vue
    │   │   │   ├── DeviceManager.vue
    │   │   │   └── ProcessDetailsModal.vue # (新增)
    │   │   ├── TheDesktop.vue
    │   │   └── TheTaskbar.vue
    │   ├── services/
    │   │   └── api.ts      # API客户端定义
    │   └── ...
    └── ...
```

---

## 🚀 快速开始 (Windows)

### 依赖环境
- **后端**: Visual Studio 2022 (带 C++17), CMake 3.15+
- **前端**: Node.js 16+, npm

### 运行步骤
1.  **启动后端服务器**:
    *   在 `backend` 目录中，使用CMake和Visual Studio进行编译。
    *   运行 `backend/build/Debug/os_simulator.exe`。
    *   服务器将在 `http://localhost:8080` 上监听。

2.  **启动前端开发服务器**:
    *   打开 **新终端**，进入 `frontend` 目录。
    *   运行 `npm install` 安装依赖。
    *   运行 `npm run dev` 启动前端服务器。
    *   在浏览器中打开显示的URL (通常是 `http://localhost:5173`)。

---

## 🔩 后端架构：C++核心

后端是模拟器的心脏，负责实现所有核心的操作系统功能。

| 模块 | 头文件 | 职责 |
| :--- | :--- | :--- |
| **进程管理** | `process_manager.h` | 创建、终止、管理进程的生命周期和状态。 |
| **内存管理** | `memory_manager.h` | 模拟连续内存分配，跟踪已用和空闲内存。 |
| **文件系统** | `filesystem.h` | 实现一个基于Inode的内存文件系统，支持文件和目录操作。 |
| **调度器** | `scheduler.h` | 实现多种调度算法 (如RR, FIFO, 优先级)，管理就绪队列。 |
| **设备管理** | `device_manager.h` | 模拟系统设备（如打印机、磁盘）的请求和释放。 |
| **中断处理** | `interrupt_handler.h`| 管理中断向量表，处理和记录中断事件。 |
| **时钟管理** | `clock_manager.h` | 模拟系统时钟，提供计时器功能。 |

---

## 🖥️ 前端指南：图形用户界面

前端提供了一个交互式的桌面环境来可视化和控制后端的OS内核。

### 主要应用与功能

#### 1. 进程管理器 (`ProcessManager.vue`)
一个强大的工具，用于深入了解和控制进程与调度。
- **进程列表**: 实时查看所有进程的状态、PID和内存使用。
- **调度器控制**:
    - **算法切换**: 通过下拉菜单动态更改调度算法 (RR, FIFO, 优先级)。
    - **就绪队列**: 在侧边栏实时查看等待调度的就绪队列。
- **进程操作**:
    - **终止进程**: 强制结束任何正在运行的进程。
    - **查看详情**: 点击"详情"按钮，会弹出一个模态窗口 (`ProcessDetailsModal.vue`)，显示特定进程的详细信息，如内存块地址和大小。

#### 2. 设备管理器 (`DeviceManager.vue`)
一个全新的应用，用于与系统设备进行交互。
- **设备列表**: 查看所有可用设备及其当前状态（空闲或繁忙）。
- **设备详情**: 点击一个设备，右侧会显示其详细信息，如所有者进程ID。
- **I/O操作**: 对选定的设备执行模拟的I/O操作，如 **请求**、**释放**、**读取** 和 **写入**，并查看其状态变化。

#### 3. 终端 (`Terminal.vue`)
一个功能齐全的命令行界面，提供了对底层API的直接访问。
- **文件系统**: `ls`, `cd`, `pwd`, `cat`, `mkdir`, `touch`, `rm`
- **进程管理**: `ps`, `kill`, `run`
- **系统信息**: `free`, `lsdev`, `time`, `uptime`
- **中断管理**:
    - `register`, `trigger`, `inttable`
    - `inthistory` (**新功能**): 查看最近的中断历史记录。
- **其他**: `help`, `clear`

#### 4. 文件管理器 (`FileManager.vue`)
- 提供图形化的文件和目录导航，支持所有基本的文件操作（创建、删除、重命名、读写）。

---

## 🌐 API接口

前后端通过一套RESTful API进行通信。所有API的请求和响应格式都在 `backend/doc/API.md` 中有详细定义。前端通过 `frontend/src/services/api.ts` 文件中的类型化客户端与这些端点交互。

## 💡 系统初始状态

为了更好地模拟一个正在运行的操作系统，模拟器在启动时会创建一组默认的进程、文件和目录。这对于前端开发和API测试非常重要。

### 初始进程
- **idle_process** (PID 1): 一个低内存占用的空闲进程。
- **system_logger** (PID 2): 模拟的日志服务进程。
- **shell** (PID 3): 模拟的用户交互终端进程。

### 初始文件系统
```
/
├── home/
│   ├── welcome.txt
│   └── myapp.pubt          # 用户自定义应用
├── etc/
│   └── config.conf
├── bin/
│   ├── calculator.pubt     # 计算器程序
│   ├── notepad.pubt        # 记事本程序
│   ├── browser.pubt        # 浏览器程序
│   └── game.pubt           # 游戏程序
├── var/
│   └── log/
│       └── system.log
└── tmp/                    # 临时文件目录
```

## 🎯 使用示例

### 终端操作示例

```bash
# 查看当前目录
[/]> pwd
/

# 列出根目录内容
[/]> ls
📁 home                 <DIR>     2024-01-01 12:00:00
📁 etc                  <DIR>     2024-01-01 12:00:00
📁 bin                  <DIR>     2024-01-01 12:00:00
📁 var                  <DIR>     2024-01-01 12:00:00

# 进入bin目录
[/]> cd bin
Changed directory to /bin

# 查看可执行文件
[/bin]> ls
📄 calculator.pubt      1024   2024-01-01 12:00:00
📄 notepad.pubt         2048   2024-01-01 12:00:00
📄 browser.pubt         4096   2024-01-01 12:00:00
📄 game.pubt            8192   2024-01-01 12:00:00

# 查看系统进程
[/bin]> ps
PID    NAME           MEMORY   STATE
----   -------------- -------- ---------
1      idle_process   100      running
2      system_logger  200      running
3      shell          150      running

# 创建新进程
[/bin]> run calculator 1024
Process created: calculator (PID: 4)

# 查看内存状态
[/bin]> free
Memory Status:
Total:     8192 KB
Used:      1470 KB
Free:      6722 KB
Usage:     17.9%

Allocations:
  PID 1: 100 KB
  PID 2: 200 KB
  PID 3: 150 KB
  PID 4: 1024 KB

# 设置中断处理程序
[/bin]> register 0 timer 1
Interrupt handler registered successfully

# 触发中断
[/bin]> trigger 0 {"message": "test"}
Interrupt triggered successfully

# 查看中断表
[/bin]> inttable
Vector | Type    | Priority | Status
-------|---------|----------|--------
0      | timer   | 1        | active
```

## ⚠️ 注意事项

### 系统要求
- 确保后端服务器在前端启动之前已经运行
- 前端默认连接到 `http://localhost:8080` 的后端API
- 如果端口冲突，请修改相应的配置文件

### 常见问题
1. **前端无法连接后端**: 检查后端服务器是否正常运行，端口是否正确
2. **命令无响应**: 确保API调用格式正确，查看浏览器控制台错误信息
3. **文件操作失败**: 检查文件路径是否正确，是否有相应权限

### 开发建议
- 使用浏览器开发者工具查看网络请求和API响应
- 查看后端日志了解服务器处理情况
- 参考 `backend/doc/API.md` 了解完整的API接口文档

## 🤝 如何贡献

我们欢迎所有形式的贡献！无论是添加新功能、修复Bug，还是完善文档。

1.  **Fork** 本仓库。
2.  创建您的特性分支 (`git checkout -b feature/AmazingFeature`)。
3.  提交您的更改 (`git commit -m 'Add some AmazingFeature'`)。
4.  **为您的更改编写或更新测试**，并确保所有测试都能通过。
5.  将您的分支推送到远程 (`git push origin feature/AmazingFeature`)。
6.  创建一个新的 **Pull Request**。

--- 