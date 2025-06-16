# 操作系统模拟器 (OS Simulator)

## 📖 项目简介

本项目是一个功能完备的操作系统模拟器，为操作系统课程设计的大作业。通过 C++ 后端实现核心 OS 功能，由 Vue.js 前端提供类似 Windows 桌面的图形用户界面。该模拟器实现了操作系统的基本功能，包括进程管理、内存管理、文件系统、设备管理、中断处理等核心模块。

### ✨ 核心特色

- **🔗 前后端分离**: C++ 后端专注于模拟操作系统核心逻辑，Vue.js 前端提供美观、交互式的用户界面
- **🧩 模块化设计**: 后端逻辑高度模块化，涵盖进程、内存、文件、设备、时钟、中断和调度等核心功能
- **🖥️ 图形化界面**: 提供类似 Windows 的桌面环境，包括开始菜单、任务栏、窗口管理和实时系统监控
- **🎯 功能丰富**: 内置文件管理器、进程管理器、设备管理器、终端等多种应用程序
- **🌐 RESTful API**: 通过清晰的 RESTful API 连接前后端，便于扩展和测试
- **⚡ 实时监控**: 动态展示系统运行期间的快照，包括进程状态、内存分配情况、设备状态等

---

## 🏗️ 项目架构

```
.
├── backend/                    # C++ 后端 (操作系统核心逻辑)
│   ├── include/                # 头文件目录
│   │   ├── common.h           # 通用定义和结构
│   │   ├── process/           # 进程管理头文件
│   │   ├── memory/            # 内存管理头文件
│   │   ├── fs/                # 文件系统头文件
│   │   ├── device/            # 设备管理头文件
│   │   ├── interrupt/         # 中断处理头文件
│   │   └── clock/             # 时钟管理头文件
│   ├── src/                   # 源代码目录
│   │   ├── core/              # 核心模块 (main.cpp, api_server.cpp)
│   │   ├── process/           # 进程管理实现
│   │   ├── memory/            # 内存管理实现
│   │   ├── fs/                # 文件系统实现
│   │   ├── device/            # 设备管理实现
│   │   ├── interrupt/         # 中断处理实现
│   │   └── clock/             # 时钟管理实现
│   ├── tests/                 # 测试文件
│   ├── doc/                   # 文档目录 (API.md)
│   ├── build/                 # 构建输出目录
│   └── CMakeLists.txt         # CMake 构建配置
└── frontend/                  # Vue.js 前端 (图形用户界面)
    ├── src/
    │   ├── components/
    │   │   ├── apps/          # 应用程序组件
    │   │   │   ├── ProcessManager.vue    # 进程管理器
    │   │   │   ├── MemoryManager.vue     # 内存管理器
    │   │   │   ├── DeviceManager.vue     # 设备管理器
    │   │   │   ├── FileManager.vue       # 文件管理器
    │   │   │   ├── Terminal.vue          # 命令行终端
    │   │   │   ├── MusicPlayer.vue       # 音乐播放器
    │   │   │   └── SystemControl.vue     # 系统控制面板
    │   │   ├── TheDesktop.vue             # 桌面组件
    │   │   ├── TheTaskbar.vue             # 任务栏组件
    │   │   ├── AppWindow.vue              # 应用窗口组件
    │   │   └── BootLoader.vue             # 启动加载器
    │   ├── services/
    │   │   └── api.ts         # API 客户端定义
    │   ├── stores/            # Pinia 状态管理
    │   ├── router/            # Vue Router 路由配置
    │   └── utils/             # 工具函数
    ├── public/                # 静态资源
    ├── package.json           # npm 依赖配置
    └── vite.config.ts         # Vite 构建配置
```

---

## 🚀 环境配置与运行指南

### 系统要求

- **操作系统**: Windows 11
- **命令行**: PowerShell
- **内存设定**: 模拟器设定内存为 4GB

### 前置依赖

#### 后端依赖
- **Visual Studio 2022** (包含 C++ 17 支持)
- **CMake 3.10+**
- **Git for Windows**

#### 前端依赖
- **Node.js 16.0+**
- **npm 或 yarn**

### 📦 安装步骤

#### 1. 克隆项目
```powershell
git clone <your-repository-url>
cd OS
```

#### 2. 后端构建

打开 PowerShell，进入后端目录：

```powershell
# 进入后端目录
cd backend

# 创建构建目录
if (!(Test-Path "build")) { mkdir build }
cd build

# 使用 CMake 生成项目文件
cmake .. 

# 编译项目
cmake --build . 
# 或者使用 Release 模式
# cmake --build . --config Release
```

#### 3. 前端构建

打开新的 PowerShell 窗口，进入前端目录：

```powershell
# 进入前端目录
cd frontend

# 安装依赖
npm install

# 启动开发服务器
npm run dev
```

### 🚀 运行项目

#### 启动后端服务器

在后端构建完成后：

```powershell
# 在 backend/build 目录下
cd Debug  # 或 Release

# 运行后端服务器
.\os_simulator.exe
```

后端服务器将在 `http://localhost:8080` 启动。

#### 启动前端界面

在前端目录下：

```powershell
# 确保在 frontend 目录
npm run dev
```

前端开发服务器通常在 `http://localhost:5173` 启动。

### 🔧 运行脚本 (推荐)

为方便使用，可以创建启动脚本：

**start_backend.ps1** (放在项目根目录):
```powershell
# 启动后端服务器
Write-Host "正在启动操作系统模拟器后端..." -ForegroundColor Green
cd backend\build\Debug
Start-Process -FilePath ".\os_simulator.exe" -NoNewWindow
Write-Host "后端服务器已启动在 http://localhost:8080" -ForegroundColor Yellow
```

**start_frontend.ps1** (放在项目根目录):
```powershell
# 启动前端开发服务器
Write-Host "正在启动前端界面..." -ForegroundColor Green
cd frontend
npm run dev
```

**start_all.ps1** (一键启动):
```powershell
# 一键启动前后端
Write-Host "正在启动操作系统模拟器..." -ForegroundColor Cyan

# 启动后端
Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd '$PWD'; .\start_backend.ps1"

# 等待后端启动
Start-Sleep -Seconds 3

# 启动前端
Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd '$PWD'; .\start_frontend.ps1"

Write-Host "系统启动完成！" -ForegroundColor Green
Write-Host "后端: http://localhost:8080" -ForegroundColor Yellow
Write-Host "前端: http://localhost:5173" -ForegroundColor Yellow
```

使用方法：
```powershell
# 设置执行策略（首次运行需要）
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# 一键启动
.\start_all.ps1
```

---

## 🧩 核心功能模块

### 后端核心模块

| 模块 | 文件路径 | 主要功能 |
|------|----------|----------|
| **进程管理** | `src/process/` | 进程创建、调度、状态管理、进程间通信 |
| **内存管理** | `src/memory/` | 内存分配与回收、连续内存管理、进程交换 |
| **文件系统** | `src/fs/` | 基于 inode 的文件系统、目录管理、文件操作 |
| **设备管理** | `src/device/` | 设备申请、分配、使用、释放，设备队列管理 |
| **中断处理** | `src/interrupt/` | 中断向量表、中断处理程序、中断机制模拟 |
| **时钟管理** | `src/clock/` | 系统时钟、定时器功能 |
| **调度器** | `src/process/scheduler.cpp` | FCFS、SJF、优先级、时间片轮转等算法 |

### 前端应用程序

| 应用 | 组件文件 | 功能描述 |
|------|----------|----------|
| **进程管理器** | `ProcessManager.vue` | 查看进程列表、调度算法切换、进程操作 |
| **内存管理器** | `MemoryManager.vue` | 内存分配情况、内存块可视化 |
| **设备管理器** | `DeviceManager.vue` | 设备状态查看、I/O 操作控制 |
| **文件管理器** | `FileManager.vue` | 文件浏览、文件操作、目录管理 |
| **终端** | `Terminal.vue` | 命令行界面、系统命令执行 |
| **系统控制** | `SystemControl.vue` | 系统配置、运行参数设置 |

---

## 💻 终端命令参考

模拟器内置了丰富的命令行接口，支持以下命令类别：

### 文件系统命令
```bash
ls                 # 列出当前目录内容
cd <path>          # 切换目录
pwd                # 显示当前目录路径
mkdir <name>       # 创建目录
touch <filename>   # 创建文件
rm <name>          # 删除文件或目录
cat <filename>     # 查看文件内容
```

### 进程管理命令
```bash
ps                 # 显示进程列表
kill <pid>         # 终止指定进程
run <name> <size>  # 创建新进程
jobs               # 显示作业列表
```

### 系统信息命令
```bash
free               # 显示内存使用情况
lsdev              # 显示设备列表
time               # 显示当前时间
uptime             # 显示系统运行时间
```

### 中断管理命令
```bash
register <vector> <type> <priority>  # 注册中断处理程序
trigger <vector> <data>              # 触发中断
inttable                             # 显示中断向量表
inthistory                           # 显示中断历史
```

### 其他命令
```bash
help               # 显示帮助信息
clear              # 清屏
exit               # 退出终端
```

---

## 🌐 API 接口文档

完整的 API 接口文档请参考：`backend/doc/API.md`

### 主要 API 端点

**基础 URL**: `http://localhost:8080/api/v1`

| 功能模块 | HTTP 方法 | 端点路径 | 描述 |
|----------|-----------|----------|------|
| 进程管理 | GET | `/processes` | 获取所有进程 |
| | POST | `/processes` | 创建新进程 |
| | DELETE | `/processes/{pid}` | 终止进程 |
| 内存管理 | GET | `/memory` | 获取内存状态 |
| | POST | `/memory/allocate` | 分配内存 |
| | DELETE | `/memory/free/{address}` | 释放内存 |
| 文件系统 | GET | `/filesystem/ls` | 列出目录 |
| | POST | `/filesystem/mkdir` | 创建目录 |
| | DELETE | `/filesystem/rm` | 删除文件 |
| 设备管理 | GET | `/devices` | 获取设备列表 |
| | POST | `/devices/{id}/request` | 请求设备 |
| | DELETE | `/devices/{id}/release` | 释放设备 |

---

## 🔄 系统初始状态

### 默认进程
系统启动时会创建以下默认进程：

| PID | 进程名 | 内存占用 | 状态 | 描述 |
|-----|--------|----------|------|------|
| 1 | idle_process | 100 KB | RUNNING | 空闲进程 |
| 2 | system_logger | 200 KB | RUNNING | 系统日志服务 |
| 3 | shell | 150 KB | RUNNING | 命令解释器 |

### 初始文件系统结构
```
/
├── home/
│   ├── welcome.txt                # 欢迎文件
│   └── myapp.pubt                # 用户应用程序
├── etc/
│   └── config.conf               # 系统配置文件
├── bin/
│   ├── calculator.pubt           # 计算器程序
│   ├── notepad.pubt             # 记事本程序
│   ├── browser.pubt             # 浏览器程序
│   └── game.pubt                # 游戏程序
├── var/
│   └── log/
│       └── system.log           # 系统日志
└── tmp/                         # 临时文件目录
```

---

## 🧪 测试

### 运行单元测试
```powershell
# 在 backend/build 目录下
.\Debug\unit_tests.exe
```

### 运行 API 集成测试
```powershell
# 确保后端服务器正在运行
.\Debug\api_tests.exe
```

### 使用 CTest
```powershell
# 在 backend/build 目录下
ctest -C Debug --verbose
```

---

## ⚠️ 注意事项

### 运行要求
1. **启动顺序**: 必须先启动后端服务器，再启动前端界面
2. **端口占用**: 确保 8080 和 5173 端口未被占用
3. **防火墙**: 可能需要允许应用程序通过 Windows 防火墙

### 常见问题排查

#### 后端编译失败
```powershell
# 检查 Visual Studio 和 CMake 版本
cmake --version
# 清理重新构建
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

#### 前端启动失败
```powershell
# 清理 node_modules 重新安装
Remove-Item -Recurse -Force node_modules
Remove-Item package-lock.json
npm install
```

#### 前后端连接失败
1. 检查后端服务器是否正常运行
2. 检查防火墙设置
3. 验证 API 端点是否正确响应：
```powershell
# 使用 curl 测试 API
curl http://localhost:8080/api/v1/processes
```

### 性能优化建议
- 使用 Release 模式编译后端获得更好性能
- 关闭浏览器开发者工具以提升前端性能
- 监控系统资源使用情况

---

## 📈 功能演示

### 进程管理演示
1. 打开进程管理器查看当前进程
2. 创建新进程并观察内存分配
3. 切换不同调度算法观察调度效果
4. 终止进程并查看资源释放

### 内存管理演示
1. 通过内存管理器查看内存布局
2. 创建多个进程观察内存分配
3. 终止进程观察内存碎片整理

### 文件系统演示
1. 使用文件管理器浏览目录结构
2. 创建、删除文件和目录
3. 通过终端执行文件操作命令

### 设备管理演示
1. 查看系统设备列表
2. 模拟 I/O 操作请求
3. 观察设备队列状态变化

---

## 🤝 贡献指南

欢迎提交 Issues 和 Pull Requests！

### 开发流程
1. Fork 本仓库
2. 创建特性分支：`git checkout -b feature/AmazingFeature`
3. 提交更改：`git commit -m 'Add some AmazingFeature'`
4. 推送到分支：`git push origin feature/AmazingFeature`
5. 提交 Pull Request

### 代码规范
- 后端遵循 C++17 标准
- 前端使用 TypeScript 和 Vue 3 Composition API
- 提交前请运行测试确保代码质量

---

## 📝 许可证

本项目仅用于教育目的，为操作系统课程设计作业。

---

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 🐛 Bug 报告：提交 GitHub Issue
- 💡 功能建议：提交 GitHub Issue
- 📧 邮件联系：[你的邮箱]

---

**祝您使用愉快！🎉** 