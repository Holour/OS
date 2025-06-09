# 操作系统模拟器 (OS Simulator)

本项目是一个操作系统课程设计的实践项目，旨在通过C++和现代Web技术，设计并实现一个具有图形用户界面的操作系统模拟程序。它涵盖了从底层进程调度、内存管理到上层文件系统的核心功能，并提供了一套RESTful API以便于前后端分离开发和功能展示。

## ✨ 项目特色

- **模块化设计**: 后端采用C++实现，将进程、内存、文件系统等核心功能解耦为独立的模块，便于开发、测试和扩展。
- **模拟核心功能**: 实现了进程调度、基于首次适应算法的连续内存分配、时钟管理、中断处理和设备管理等。
- **基于Inode的文件系统**: 从零开始实现了一个内存中的、基于索引节点（Inode）的经典文件系统。
- **API驱动开发**: 定义了清晰的RESTful API，允许前端通过HTTP请求与后端模拟器交互，实现了前后端分离。
- **现代化的构建系统**: 使用CMake管理项目构建，并集成了独立的API测试套件。
- **前后端分离**: 后端专注于核心逻辑模拟，前端可使用任意现代框架（如React, Vue）构建丰富的图形化界面。

## 🏛️ 项目结构

```
.
├── backend/                # C++后端代码
│   ├── build/              # CMake构建目录 (自动生成)
│   ├── doc/                # 文档
│   │   └── API.md          # 详细的API文档
│   ├── include/            # 所有模块的头文件
│   ├── src/                # 所有模块的源文件
│   ├── tests/              # API测试代码
│   └── CMakeLists.txt      # 后端构建脚本
├── frontend/               # 前端代码 (待开发)
└── README.md               # 本文档
```

## 🚀 快速开始 (Windows)

### 依赖环境
- **Visual Studio 2022 或更高版本** (需要其附带的 MSVC C++17 编译器)
- **CMake** (版本 3.15 或更高)
- (可选) Git

### 构建项目

1.  **克隆仓库**
    ```bash
    git clone <your-repository-url>
    cd os-simulator
    ```

2.  **配置并生成构建文件**
    进入 `backend` 目录，并创建一个 `build` 目录用于存放CMake生成的构建文件。
    ```bash
    cd backend
    mkdir build
    cd build
    cmake ..
    ```

3.  **编译项目**
    在 `build` 目录下，运行以下命令来编译整个后端项目。
    ```bash
    cmake --build . --config Debug
    ```
    编译成功后，将在 `build\Debug\` 目录下生成两个关键的可执行文件：
    - `os_simulator.exe`: 后端模拟器服务器。
    - `api_tests.exe`: 完整的API测试套件。

### 运行与测试

1.  **启动后端服务器**
    首先，必须启动后端服务器。在 `build` 目录下，运行：
    ```bash
    .\Debug\os_simulator.exe
    ```
    您将看到服务器初始化的日志，并提示正在 `http://0.0.0.0:8080` 上监听。请保持此窗口运行。

2.  **运行API测试**
    打开**一个新的终端窗口**，进入 `backend\build` 目录，然后运行API测试套件：
    ```bash
    .\Debug\api_tests.exe
    ```
    测试程序会自动连接到正在运行的服务器。如果所有测试都通过，您将看到 `ALL API TESTS PASSED SUCCESSFULLY!` 的成功消息。

## 📚 API 文档

本项目通过一套RESTful API暴露后端功能。详细的API文档（包括所有端点、请求/响应格式和示例）已存放在 `backend/doc/API.md` 文件中。

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
│   └── welcome.txt
├── etc/
│   └── config.conf
├── bin/
├── var/
│   └── log/
│       └── system.log
```

## 🤝 如何贡献

我们欢迎所有形式的贡献！无论是添加新功能、修复Bug，还是完善文档。

1.  **Fork** 本仓库。
2.  创建您的特性分支 (`git checkout -b feature/AmazingFeature`)。
3.  提交您的更改 (`git commit -m 'Add some AmazingFeature'`)。
4.  **为您的更改编写或更新测试**，并确保所有测试都能通过。
5.  将您的分支推送到远程 (`git push origin feature/AmazingFeature`)。
6.  创建一个新的 **Pull Request**。

--- 