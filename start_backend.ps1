# 操作系统模拟器 - 后端启动脚本
# 此脚本用于启动 C++ 后端服务器

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "    操作系统模拟器 - 后端启动器" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# 检查是否存在构建目录
if (!(Test-Path "backend\build")) {
    Write-Host "❌ 未找到构建目录！请先构建后端项目。" -ForegroundColor Red
    Write-Host "   运行以下命令进行构建：" -ForegroundColor Yellow
    Write-Host "   cd backend" -ForegroundColor Yellow
    Write-Host "   mkdir build" -ForegroundColor Yellow
    Write-Host "   cd build" -ForegroundColor Yellow
    Write-Host "   cmake .. -G `"Visual Studio 17 2022`" -A x64" -ForegroundColor Yellow
    Write-Host "   cmake --build . --config Debug" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

# 检查可执行文件是否存在
$executablePath = "backend\build\Debug\os_simulator.exe"
if (!(Test-Path $executablePath)) {
    # 尝试 Release 版本
    $executablePath = "backend\build\Release\os_simulator.exe"
    if (!(Test-Path $executablePath)) {
        Write-Host "❌ 未找到可执行文件！" -ForegroundColor Red
        Write-Host "   请确保已成功编译后端项目。" -ForegroundColor Yellow
        Write-Host ""
        pause
        exit 1
    }
}

Write-Host "🚀 正在启动操作系统模拟器后端..." -ForegroundColor Green
Write-Host "   可执行文件: $executablePath" -ForegroundColor White
Write-Host ""

# 检查端口是否被占用
$port = 8080
$portInUse = Get-NetTCPConnection -LocalPort $port -ErrorAction SilentlyContinue
if ($portInUse) {
    Write-Host "⚠️  警告: 端口 $port 已被占用！" -ForegroundColor Yellow
    Write-Host "   请确保没有其他程序占用此端口。" -ForegroundColor Yellow
    Write-Host ""
}

try {
    # 启动后端服务器
    Write-Host "🔄 启动中..." -ForegroundColor Yellow
    & $executablePath
}
catch {
    Write-Host "❌ 启动失败！" -ForegroundColor Red
    Write-Host "   错误信息: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    pause
    exit 1
}

Write-Host ""
Write-Host "✅ 后端服务器已启动！" -ForegroundColor Green
Write-Host "   服务地址: http://localhost:8080" -ForegroundColor Yellow
Write-Host "   API 文档: backend/doc/API.md" -ForegroundColor White
Write-Host ""
Write-Host "按任意键退出..." -ForegroundColor Gray
pause 