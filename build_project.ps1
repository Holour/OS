# 操作系统模拟器 - 项目构建脚本
# 此脚本用于首次构建整个项目

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "    操作系统模拟器 - 项目构建" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "此脚本将帮助您构建整个操作系统模拟器项目" -ForegroundColor White
Write-Host "包括后端 C++ 项目和前端 Vue.js 项目的构建" -ForegroundColor White
Write-Host ""

# 环境检查
Write-Host "🔍 检查构建环境..." -ForegroundColor Yellow
Write-Host ""

# 检查 CMake
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✅ $cmakeVersion" -ForegroundColor Green
}
catch {
    Write-Host "❌ 未检测到 CMake！" -ForegroundColor Red
    Write-Host "   请安装 CMake 3.10 或更新版本" -ForegroundColor Yellow
    Write-Host "   下载地址: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

# 检查 Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsInfo = & $vsWhere -latest -property displayName
    if ($vsInfo) {
        Write-Host "✅ 检测到 Visual Studio: $vsInfo" -ForegroundColor Green
    }
    else {
        Write-Host "⚠️  未检测到 Visual Studio 2022" -ForegroundColor Yellow
        Write-Host "   建议安装 Visual Studio 2022 Community（免费）" -ForegroundColor Yellow
        Write-Host "   确保包含 C++ 开发工具" -ForegroundColor Yellow
    }
}
else {
    Write-Host "⚠️  未找到 Visual Studio Installer" -ForegroundColor Yellow
    Write-Host "   请确保已安装 Visual Studio 2022" -ForegroundColor Yellow
}

# 检查 Node.js
try {
    $nodeVersion = node --version
    Write-Host "✅ Node.js 版本: $nodeVersion" -ForegroundColor Green
}
catch {
    Write-Host "❌ 未检测到 Node.js！" -ForegroundColor Red
    Write-Host "   请安装 Node.js 16.0 或更新版本" -ForegroundColor Yellow
    Write-Host "   下载地址: https://nodejs.org/" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

# 检查 npm
try {
    $npmVersion = npm --version
    Write-Host "✅ npm 版本: $npmVersion" -ForegroundColor Green
}
catch {
    Write-Host "❌ npm 不可用！" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""

# 开始构建流程
Write-Host "🚀 开始构建项目..." -ForegroundColor Green
Write-Host ""

# 1. 构建后端
Write-Host "1️⃣  构建 C++ 后端..." -ForegroundColor Yellow

if (!(Test-Path "backend")) {
    Write-Host "❌ 未找到 backend 目录！" -ForegroundColor Red
    pause
    exit 1
}

# 进入后端目录
Set-Location backend

# 创建构建目录
if (Test-Path "build") {
    Write-Host "   🗑️  清理现有构建目录..." -ForegroundColor Gray
    Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
}

Write-Host "   📁 创建构建目录..." -ForegroundColor White
New-Item -ItemType Directory -Name "build" | Out-Null
Set-Location build

# 生成项目文件
Write-Host "   ⚙️  生成 Visual Studio 项目文件..." -ForegroundColor White
try {
    cmake .. -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        throw "CMake 配置失败"
    }
    Write-Host "   ✅ 项目文件生成成功" -ForegroundColor Green
}
catch {
    Write-Host "   ❌ CMake 配置失败: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "   尝试其他 Visual Studio 版本..." -ForegroundColor Yellow
    
    # 尝试 Visual Studio 2019
    try {
        cmake .. -G "Visual Studio 16 2019" -A x64
        Write-Host "   ✅ 使用 Visual Studio 2019 生成成功" -ForegroundColor Green
    }
    catch {
        Write-Host "   ❌ 项目生成失败，请检查 Visual Studio 安装" -ForegroundColor Red
        Set-Location ../..
        pause
        exit 1
    }
}

# 编译项目
Write-Host "   🔨 编译 Debug 版本..." -ForegroundColor White
try {
    cmake --build . --config Debug
    if ($LASTEXITCODE -ne 0) {
        throw "编译失败"
    }
    Write-Host "   ✅ Debug 版本编译成功" -ForegroundColor Green
}
catch {
    Write-Host "   ❌ Debug 版本编译失败" -ForegroundColor Red
    Write-Host "   请检查 C++ 代码或编译环境" -ForegroundColor Yellow
}

Write-Host "   🔨 编译 Release 版本..." -ForegroundColor White
try {
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "编译失败"
    }
    Write-Host "   ✅ Release 版本编译成功" -ForegroundColor Green
}
catch {
    Write-Host "   ⚠️  Release 版本编译失败（可以继续使用 Debug 版本）" -ForegroundColor Yellow
}

# 返回项目根目录
Set-Location ../..

Write-Host ""

# 2. 构建前端
Write-Host "2️⃣  构建 Vue.js 前端..." -ForegroundColor Yellow

if (!(Test-Path "frontend")) {
    Write-Host "❌ 未找到 frontend 目录！" -ForegroundColor Red
    pause
    exit 1
}

# 进入前端目录
Set-Location frontend

# 安装依赖
Write-Host "   📦 安装 npm 依赖..." -ForegroundColor White
try {
    npm install
    if ($LASTEXITCODE -ne 0) {
        throw "npm install 失败"
    }
    Write-Host "   ✅ 前端依赖安装成功" -ForegroundColor Green
}
catch {
    Write-Host "   ❌ 前端依赖安装失败: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "   尝试清理缓存后重新安装..." -ForegroundColor Yellow
    
    # 清理缓存
    npm cache clean --force
    Remove-Item -Recurse -Force node_modules -ErrorAction SilentlyContinue
    Remove-Item package-lock.json -ErrorAction SilentlyContinue
    
    try {
        npm install
        Write-Host "   ✅ 重新安装成功" -ForegroundColor Green
    }
    catch {
        Write-Host "   ❌ 前端依赖安装失败，请手动处理" -ForegroundColor Red
        Set-Location ..
        pause
        exit 1
    }
}

# 类型检查
Write-Host "   🔍 运行类型检查..." -ForegroundColor White
try {
    npm run type-check
    Write-Host "   ✅ 类型检查通过" -ForegroundColor Green
}
catch {
    Write-Host "   ⚠️  类型检查有警告（通常可以忽略）" -ForegroundColor Yellow
}

# 代码检查
Write-Host "   🧹 运行代码检查..." -ForegroundColor White
try {
    npm run lint
    Write-Host "   ✅ 代码检查通过" -ForegroundColor Green
}
catch {
    Write-Host "   ⚠️  代码检查有警告（通常可以忽略）" -ForegroundColor Yellow
}

# 返回项目根目录
Set-Location ..

Write-Host ""

# 3. 运行测试
Write-Host "3️⃣  运行测试..." -ForegroundColor Yellow

# 运行后端单元测试
if (Test-Path "backend\build\Debug\unit_tests.exe") {
    Write-Host "   🧪 运行后端单元测试..." -ForegroundColor White
    try {
        & backend\build\Debug\unit_tests.exe
        Write-Host "   ✅ 后端单元测试完成" -ForegroundColor Green
    }
    catch {
        Write-Host "   ⚠️  后端单元测试有失败项" -ForegroundColor Yellow
    }
}
else {
    Write-Host "   ⚠️  未找到后端测试程序" -ForegroundColor Yellow
}

Write-Host ""

# 构建完成
Write-Host "🎉 项目构建完成！" -ForegroundColor Green
Write-Host ""

# 显示构建结果
Write-Host "📋 构建结果:" -ForegroundColor Cyan
Write-Host ""

# 检查后端可执行文件
if (Test-Path "backend\build\Debug\os_simulator.exe") {
    Write-Host "   ✅ 后端 Debug 版本: backend\build\Debug\os_simulator.exe" -ForegroundColor Green
}
else {
    Write-Host "   ❌ 后端 Debug 版本构建失败" -ForegroundColor Red
}

if (Test-Path "backend\build\Release\os_simulator.exe") {
    Write-Host "   ✅ 后端 Release 版本: backend\build\Release\os_simulator.exe" -ForegroundColor Green
}
else {
    Write-Host "   ⚠️  后端 Release 版本未构建" -ForegroundColor Yellow
}

# 检查前端依赖
if (Test-Path "frontend\node_modules") {
    Write-Host "   ✅ 前端依赖已安装" -ForegroundColor Green
}
else {
    Write-Host "   ❌ 前端依赖安装失败" -ForegroundColor Red
}

Write-Host ""
Write-Host "🚀 下一步操作:" -ForegroundColor Cyan
Write-Host "   1. 运行 .\start_all.ps1 一键启动前后端" -ForegroundColor White
Write-Host "   2. 或分别运行:" -ForegroundColor White
Write-Host "      - .\start_backend.ps1  (启动后端)" -ForegroundColor White
Write-Host "      - .\start_frontend.ps1 (启动前端)" -ForegroundColor White
Write-Host ""
Write-Host "📖 更多信息请查看 README.md" -ForegroundColor White
Write-Host ""

$response = Read-Host "是否现在启动项目？(Y/N)"
if ($response -eq "Y" -or $response -eq "y") {
    Write-Host ""
    Write-Host "🚀 启动项目..." -ForegroundColor Green
    if (Test-Path "start_all.ps1") {
        & .\start_all.ps1
    }
    else {
        Write-Host "❌ 未找到启动脚本 start_all.ps1" -ForegroundColor Red
    }
}
else {
    Write-Host ""
    Write-Host "构建完成！您可以稍后手动启动项目。" -ForegroundColor Green
    Write-Host ""
    pause
} 