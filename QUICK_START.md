# 操作系统模拟器 - 快速开始指南

## 🚀 5 分钟快速启动

### 第一次使用

1. **确保环境**
   - Windows 11 + PowerShell
   - Visual Studio 2022 (包含 C++ 工具)
   - Node.js 16+

2. **构建项目**
   ```powershell
   # 设置执行策略（首次需要）
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   
   # 构建整个项目
   .\build_project.ps1
   ```

3. **启动系统**
   ```powershell
   # 一键启动前后端
   .\start_all.ps1
   ```

### 日常使用

```powershell
# 一键启动（推荐）
.\start_all.ps1

# 或者分开启动
.\start_backend.ps1    # 后端服务器
.\start_frontend.ps1   # 前端界面
```

## 📱 使用界面

### 主要应用

1. **进程管理器** - 查看和管理进程
2. **内存管理器** - 查看内存分配情况
3. **文件管理器** - 浏览和操作文件系统
4. **设备管理器** - 管理系统设备
5. **终端** - 命令行界面
6. **系统控制** - 系统配置

### 常用终端命令

```bash
# 文件操作
ls          # 列出文件
cd /home    # 切换目录
cat file    # 查看文件内容

# 进程操作
ps          # 查看进程
run app 1024 # 创建进程（名称 内存大小）
kill 4      # 终止进程

# 系统信息
free        # 内存状态
lsdev       # 设备列表
help        # 帮助信息
```

## 🔧 快速故障排除

### 后端无法启动
```powershell
# 重新构建后端
cd backend\build
cmake --build . --config Debug
```

### 前端无法连接
```powershell
# 检查后端是否运行
curl http://localhost:8080/api/v1/processes
```

### 端口冲突
- 后端默认端口：8080
- 前端默认端口：5173
- 确保这些端口未被占用

## 📋 系统默认状态

### 初始进程
- **PID 1**: idle_process（空闲进程）
- **PID 2**: system_logger（日志服务）
- **PID 3**: shell（命令解释器）

### 文件系统
```
/
├── home/     # 用户目录
├── bin/      # 可执行程序
├── etc/      # 配置文件
└── var/      # 日志文件
```

## 🎯 核心功能体验

1. **进程调度演示**
   - 打开进程管理器
   - 创建多个进程
   - 切换调度算法观察效果

2. **内存管理演示**
   - 查看内存分配图
   - 创建和终止进程观察内存变化

3. **文件系统演示**
   - 使用文件管理器创建文件夹
   - 通过终端执行文件操作

4. **设备模拟演示**
   - 在设备管理器中请求设备
   - 观察设备队列状态

## 📞 获取帮助

- 📖 详细文档：查看 `README.md`
- 🌐 API 文档：`backend/doc/API.md`
- 🔧 终端帮助：在终端中输入 `help`

## ⚡ 性能建议

- 使用 Release 版本获得更好性能
- 关闭浏览器开发者工具
- 不要同时运行过多进程

---

**开始探索您的操作系统模拟器吧！🎉** 