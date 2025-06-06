# 操作系统模拟器 (OS Simulator)

本项目是一个操作系统课程设计的实践项目，旨在通过C++和现代Web技术，设计并实现一个具有图形用户界面的操作系统模拟程序。它涵盖了从底层进程调度、内存管理到上层文件系统的核心功能，并提供了一套RESTful API以便于前后端分离开发和功能展示。

## ✨ 项目特色

- **模块化设计**: 后端采用C++实现，将进程、内存、文件系统等核心功能解耦为独立的模块，便于开发、测试和扩展。
- **真实的内存管理**: 模拟了基于首次适应算法的连续内存分配，以及带有真实内存池的块式管理。
- **基于Inode的文件系统**: 从零开始实现了一个内存中的、基于索引节点（Inode）的经典文件系统。
- **API驱动开发**: 定义了清晰的RESTful API，允许前端通过HTTP请求与后端模拟器交互，实现了前后端分离。
- **现代化的构建系统**: 使用CMake管理项目构建，并集成了CTest单元测试框架，保证代码质量。
- **前后端分离**: 后端专注于核心逻辑模拟，前端（待开发）可使用任意现代框架（如React, Vue）构建丰富的图形化界面。

## 🏛️ 项目结构

```
.
├── backend/                # C++后端代码
│   ├── build/              # CMake构建目录 (自动生成)
│   ├── include/            # 所有模块的头文件
│   │   ├── common.h
│   │   ├── device/
│   │   ├── fs/
│   │   ├── interrupt/
│   │   ├── memory/
│   │   └── process/
│   ├── src/                # 所有模块的源文件
│   │   ├── core/           # 主程序入口 (main.cpp)
│   │   ├── device/
│   │   ├── fs/
│   │   ├── interrupt/
│   │   ├── memory/
│   │   └── process/
│   ├── tests/              # 单元测试代码
│   └── CMakeLists.txt      # 后端构建脚本
├── frontend/               # 前端代码 (待开发)
└── README.md               # 本文档
```

## 🚀 快速开始

### 依赖环境
- C++ 编译器 (支持C++17标准, 如 MSVC, GCC, Clang)
- CMake (版本 3.10 或更高)
- (可选) Git

### 构建与运行

1.  **克隆仓库**
    ```bash
    git clone <your-repository-url>
    cd os-simulator
    ```

2.  **创建构建目录**
    进入 `backend` 目录，并创建一个 `build` 目录用于存放CMake生成的构建文件。
    ```bash
    cd backend
    mkdir build
    cd build
    ```

3.  **生成构建文件**
    运行CMake来配置项目并生成适用于您本地环境的构建文件（例如Visual Studio解决方案或Makefile）。
    ```bash
    cmake ..
    ```

4.  **编译项目**
    使用CMake来调用底层的构建工具（如MSBuild或make）进行编译。
    ```bash
    cmake --build .
    ```
    编译成功后，将在 `build/Debug/` 目录下生成三个可执行文件：
    - `os_simulator.exe`: 主程序（当前为空）。
    - `process_test.exe`: 进程管理模块的测试程序。
    - `memory_test.exe`: 内存管理模块的测试程序。
    - `fs_test.exe`: 文件系统模块的测试程序。

5.  **运行单元测试**
    我们使用CTest来管理和运行单元测试。在 `build` 目录下，运行：
    ```bash
    ctest -C Debug
    ```
    如果所有测试都通过，您将看到类似以下的输出，表明后端核心功能工作正常：
    ```
    100% tests passed, 0 tests failed out of 3
    ```

## 📚 API 文档

本项目通过一套RESTful API暴露后端功能。详细的API文档请参考 `doc/API.md` (待创建) 或咨询主要开发者。
*(注: 我们可以将上一条消息生成的API文档放入该路径下)*

## 🤝 如何贡献

我们欢迎所有形式的贡献！无论是添加新功能、修复Bug，还是完善文档。

1.  **Fork** 本仓库。
2.  创建您的特性分支 (`git checkout -b feature/AmazingFeature`)。
3.  提交您的更改 (`git commit -m 'Add some AmazingFeature'`)。
4.  **为您的更改编写单元测试**，并确保所有测试都能通过。
5.  将您的分支推送到远程 (`git push origin feature/AmazingFeature`)。
6.  创建一个新的 **Pull Request**。

--- 