# 操作系统模拟器 - 一键启动脚本
# 此脚本将同时启动后端和前端服务

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "     操作系统模拟器 - 一键启动" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# 检查必要的脚本是否存在
$backendScript = "start_backend.ps1"
$frontendScript = "start_frontend.ps1"

if (!(Test-Path $backendScript)) {
    Write-Host "❌ 未找到后端启动脚本: $backendScript" -ForegroundColor Red
    Write-Host ""
    pause
    exit 1
}

if (!(Test-Path $frontendScript)) {
    Write-Host "❌ 未找到前端启动脚本: $frontendScript" -ForegroundColor Red
    Write-Host ""
    pause
    exit 1
}

# 检查执行策略
$executionPolicy = Get-ExecutionPolicy
if ($executionPolicy -eq "Restricted") {
    Write-Host "⚠️  PowerShell 执行策略限制！" -ForegroundColor Yellow
    Write-Host "   请运行以下命令设置执行策略：" -ForegroundColor Yellow
    Write-Host "   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser" -ForegroundColor White
    Write-Host ""
    
    $response = Read-Host "是否现在设置执行策略？(Y/N)"
    if ($response -eq "Y" -or $response -eq "y") {
        try {
            Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force
            Write-Host "✅ 执行策略已设置" -ForegroundColor Green
        }
        catch {
            Write-Host "❌ 设置执行策略失败" -ForegroundColor Red
            pause
            exit 1
        }
    }
    else {
        Write-Host "操作已取消" -ForegroundColor Yellow
        pause
        exit 1
    }
    
    Write-Host ""
}

Write-Host "🚀 正在启动操作系统模拟器..." -ForegroundColor Green
Write-Host ""

# 启动后端服务器 (在新窗口中)
Write-Host "1️⃣  启动后端服务器..." -ForegroundColor Yellow
try {
    $backendProcess = Start-Process powershell -ArgumentList "-NoExit", "-File", $backendScript -PassThru
    Write-Host "   ✅ 后端服务器启动窗口已打开 (PID: $($backendProcess.Id))" -ForegroundColor Green
}
catch {
    Write-Host "   ❌ 后端启动失败: $($_.Exception.Message)" -ForegroundColor Red
    pause
    exit 1
}

# 等待后端启动
Write-Host ""
Write-Host "⏳ 等待后端服务器启动..." -ForegroundColor Yellow
Write-Host "   (通常需要 3-5 秒)" -ForegroundColor Gray

$maxWaitTime = 15
$waitTime = 0
$backendReady = $false

while ($waitTime -lt $maxWaitTime -and !$backendReady) {
    Start-Sleep -Seconds 1
    $waitTime++
    
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:8080/api/v1/processes" -TimeoutSec 2 -ErrorAction Stop
        $backendReady = $true
        Write-Host "   ✅ 后端服务器就绪！" -ForegroundColor Green
    }
    catch {
        Write-Host "   ⏳ 等待中... ($waitTime/$maxWaitTime 秒)" -ForegroundColor Gray
    }
}

if (!$backendReady) {
    Write-Host "   ⚠️  后端服务器启动超时，但仍会尝试启动前端" -ForegroundColor Yellow
}

# 启动前端开发服务器 (在新窗口中)
Write-Host ""
Write-Host "2️⃣  启动前端开发服务器..." -ForegroundColor Yellow
try {
    $frontendProcess = Start-Process powershell -ArgumentList "-NoExit", "-File", $frontendScript -PassThru
    Write-Host "   ✅ 前端服务器启动窗口已打开 (PID: $($frontendProcess.Id))" -ForegroundColor Green
}
catch {
    Write-Host "   ❌ 前端启动失败: $($_.Exception.Message)" -ForegroundColor Red
    # 不退出，让用户手动启动前端
}

Write-Host ""
Write-Host "🎉 启动完成！" -ForegroundColor Green
Write-Host ""
Write-Host "📋 服务信息:" -ForegroundColor Cyan
Write-Host "   🔗 后端 API:  http://localhost:8080" -ForegroundColor Yellow
Write-Host "   🖥️  前端界面: http://localhost:5173 (通常)" -ForegroundColor Yellow
Write-Host ""
Write-Host "📁 项目结构:" -ForegroundColor Cyan
Write-Host "   📄 API 文档: backend/doc/API.md" -ForegroundColor White
Write-Host "   🧪 运行测试: backend/build/Debug/unit_tests.exe" -ForegroundColor White
Write-Host "   📊 前端更新: frontend/UPDATE_SUMMARY.md" -ForegroundColor White
Write-Host ""
Write-Host "💡 使用提示:" -ForegroundColor Cyan
Write-Host "   • 首次使用建议先阅读 README.md" -ForegroundColor White
Write-Host "   • 前端通常会自动打开浏览器" -ForegroundColor White
Write-Host "   • 如果遇到问题，请检查对应的启动窗口输出" -ForegroundColor White
Write-Host "   • 可以通过任务管理器关闭相关进程" -ForegroundColor White
Write-Host ""
Write-Host "⚠️  注意事项:" -ForegroundColor Yellow
Write-Host "   • 不要关闭后端服务器窗口，否则前端将无法工作" -ForegroundColor White
Write-Host "   • 端口 8080 和 5173 需要保持空闲" -ForegroundColor White
Write-Host "   • 建议关闭防火墙警告或允许通过防火墙" -ForegroundColor White
Write-Host ""

# 尝试自动打开前端页面
Write-Host "🌐 尝试自动打开前端页面..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

try {
    # 先尝试检测前端是否已启动
    $attempts = 0
    $maxAttempts = 10
    $frontendReady = $false
    
    while ($attempts -lt $maxAttempts -and !$frontendReady) {
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:5173" -TimeoutSec 2 -ErrorAction Stop
            $frontendReady = $true
        }
        catch {
            $attempts++
            Start-Sleep -Seconds 2
        }
    }
    
    if ($frontendReady) {
        Start-Process "http://localhost:5173"
        Write-Host "   ✅ 浏览器已打开前端页面" -ForegroundColor Green
    }
    else {
        Write-Host "   ⚠️  前端页面尚未就绪，请稍后手动访问" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "   ℹ️  请手动在浏览器中访问 http://localhost:5173" -ForegroundColor White
}

Write-Host ""
Write-Host "✨ 系统启动流程完成！" -ForegroundColor Green
Write-Host "   现在可以开始使用操作系统模拟器了。" -ForegroundColor White
Write-Host ""
Write-Host "按任意键关闭此窗口..." -ForegroundColor Gray
pause 