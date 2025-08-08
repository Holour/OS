Param(
  [ValidateSet("Debug", "Release")] [string]$BackendConfig = "Debug",
  [int]$BackendPort = 8080,
  [switch]$UseCtest,
  [switch]$FailOnFail
)

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$ErrorActionPreference = 'Stop'

function Write-Info($m){ Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Write-Warn($m){ Write-Host "[WARN] $m" -ForegroundColor Yellow }
function Write-Err($m){ Write-Host "[ERR ] $m" -ForegroundColor Red }

function Ensure-Dir($path){ if (-not (Test-Path -LiteralPath $path)) { New-Item -ItemType Directory -Path $path | Out-Null } }

function Invoke-CMake([string]$buildDir, [string]$config){
  Ensure-Dir $buildDir
  Push-Location $buildDir
  try {
    Write-Info "Configuring CMake (VS2022 x64)"
    cmake .. -G "Visual Studio 17 2022" -A x64
    Write-Info "Building backend ($config)"
    cmake --build . --config $config
  } finally { Pop-Location }
}

function Resolve-Exe([string]$buildDir, [string]$config, [string]$name){
  $candidate1 = Join-Path $buildDir (Join-Path $config $name)
  $candidate2 = Join-Path $buildDir $name
  if (Test-Path $candidate1) { return (Resolve-Path $candidate1).Path }
  if (Test-Path $candidate2) { return (Resolve-Path $candidate2).Path }
  return $null
}

function Start-Backend([string]$config, [int]$port){
  $buildDir = Join-Path $PSScriptRoot 'backend/build'
  Invoke-CMake $buildDir $config
  $exe = Resolve-Exe $buildDir $config 'os_simulator.exe'
  if (-not $exe) { throw "未找到 os_simulator.exe" }
  Write-Info "启动后端: $exe"
  $proc = Start-Process -FilePath $exe -PassThru -WindowStyle Minimized
  $global:BackendPid = $proc.Id
  # 等待健康
  $health = "http://127.0.0.1:$port/health"
  for($i=0;$i -lt 60;$i++){
    try { $r = Invoke-WebRequest -Uri $health -TimeoutSec 3 -UseBasicParsing; if($r.StatusCode -eq 200){ break } } catch {}
    Start-Sleep -Milliseconds 500
  }
}

function Stop-Backend(){
  if ($global:BackendPid) {
    Write-Info "停止后端 PID=$($global:BackendPid)"
    try { Stop-Process -Id $global:BackendPid -Force -ErrorAction Stop } catch {}
  }
}

function Run-Ctest([string]$buildDir, [string]$config){
  Push-Location $buildDir
  try { ctest -C $config --output-on-failure --verbose } finally { Pop-Location }
}

# 全局不启用 trap，改为局部 try/catch，避免 PS 版本差异导致的异常中断

# --- Orchestrate tests ---
$buildDir = Join-Path $PSScriptRoot 'backend/build'
Invoke-CMake $buildDir $BackendConfig

# 运行单元测试（无需后端进程）
$unit = Resolve-Exe $buildDir $BackendConfig 'unit_tests.exe'
if (-not $unit) { throw "未找到 unit_tests.exe" }
Write-Info "运行单元测试: $unit"
& $unit
$unitExit = $LASTEXITCODE
Write-Info "单元测试退出码: $unitExit"

if ($UseCtest) { Write-Info '运行 CTest'; Run-Ctest $buildDir $BackendConfig }

# 启动后端，运行 API 集成测试
Start-Backend -config $BackendConfig -port $BackendPort
try {
  $api = Resolve-Exe $buildDir $BackendConfig 'api_tests.exe'
  if (-not $api) { throw "未找到 api_tests.exe" }
  Write-Info "运行 API 集成测试: $api"
  & $api
  $apiExit = $LASTEXITCODE
  Write-Info "API 测试退出码: $apiExit"
} finally {
  Stop-Backend
}

if (($unitExit -ne 0) -or ($apiExit -ne 0)) {
  Write-Warn "部分测试失败。Unit=$unitExit, API=$apiExit"
  if ($FailOnFail) { exit 1 } else { exit 0 }
}

Write-Host "`nAll tests passed." -ForegroundColor Green

 