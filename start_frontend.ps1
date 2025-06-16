# 操作系统模拟器 - 前端启动脚本
# 此脚本用于启动 Vue.js 前端开发服务器

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "    操作系统模拟器 - 前端启动器" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# 检查 Node.js 是否安装
try {
    $nodeVersion = node --version
    Write-Host "✅ Node.js 版本: $nodeVersion" -ForegroundColor Green
}
catch {
    Write-Host "❌ 未检测到 Node.js！" -ForegroundColor Red
    Write-Host "   请先安装 Node.js 16.0 或更新版本。" -ForegroundColor Yellow
    Write-Host "   下载地址: https://nodejs.org/" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

# 检查 npm 是否可用
try {
    $npmVersion = npm --version
    Write-Host "✅ npm 版本: $npmVersion" -ForegroundColor Green
}
catch {
    Write-Host "❌ npm 不可用！" -ForegroundColor Red
    Write-Host "   请重新安装 Node.js。" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

Write-Host ""

# 检查前端目录是否存在
if (!(Test-Path "frontend")) {
    Write-Host "❌ 未找到前端目录！" -ForegroundColor Red
    Write-Host "   请确保在项目根目录运行此脚本。" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

# 进入前端目录
Set-Location frontend

# 检查 package.json 是否存在
if (!(Test-Path "package.json")) {
    Write-Host "❌ 未找到 package.json 文件！" -ForegroundColor Red
    Write-Host "   前端项目配置文件缺失。" -ForegroundColor Yellow
    Write-Host ""
    Set-Location ..
    pause
    exit 1
}

# 检查是否已安装依赖
if (!(Test-Path "node_modules")) {
    Write-Host "📦 未检测到 node_modules，正在安装依赖..." -ForegroundColor Yellow
    Write-Host ""
    
    try {
        npm install
        Write-Host ""
        Write-Host "✅ 依赖安装完成！" -ForegroundColor Green
    }
    catch {
        Write-Host "❌ 依赖安装失败！" -ForegroundColor Red
        Write-Host "   错误信息: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host ""
        Set-Location ..
        pause
        exit 1
    }
}
else {
    Write-Host "✅ 依赖已安装" -ForegroundColor Green
}

Write-Host ""

# 检查后端服务器是否运行
Write-Host "🔍 检查后端服务器连接..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8080/api/v1/processes" -TimeoutSec 3 -ErrorAction Stop
    Write-Host "✅ 后端服务器正常运行" -ForegroundColor Green
}
catch {
    Write-Host "⚠️  警告: 无法连接到后端服务器！" -ForegroundColor Yellow
    Write-Host "   请确保后端服务器已启动在 http://localhost:8080" -ForegroundColor Yellow
    Write-Host "   可以运行 .\start_backend.ps1 启动后端服务器" -ForegroundColor White
}

Write-Host ""

# 启动前端开发服务器
Write-Host "🚀 正在启动前端开发服务器..." -ForegroundColor Green
Write-Host "   这可能需要几秒钟时间..." -ForegroundColor White
Write-Host ""

try {
    # 启动开发服务器
    npm run dev
}
catch {
    Write-Host "❌ 前端启动失败！" -ForegroundColor Red
    Write-Host "   错误信息: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Set-Location ..
    pause
    exit 1
}

# 返回项目根目录
Set-Location ..

Write-Host ""
Write-Host "✅ 前端开发服务器已启动！" -ForegroundColor Green
Write-Host "   通常运行在: http://localhost:5173" -ForegroundColor Yellow
Write-Host "   后端 API: http://localhost:8080" -ForegroundColor Yellow
Write-Host ""
Write-Host "按任意键退出..." -ForegroundColor Gray
pause 