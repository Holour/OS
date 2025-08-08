Param(
  [ValidateSet("Debug", "Release")]
  [string]$BackendConfig = "Debug",

  [ValidateSet("dev", "preview")]
  [string]$FrontendMode = "dev",

  [int]$BackendPort = 8080
)

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$ErrorActionPreference = 'Stop'

function Write-Info($msg) { Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Warn($msg) { Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err($msg) { Write-Host "[ERR ] $msg" -ForegroundColor Red }

function Ensure-Dir($path) {
  if (-not (Test-Path -LiteralPath $path)) { New-Item -ItemType Directory -Path $path | Out-Null }
}

function Invoke-CMake-Configure([string]$buildDir) {
  Push-Location $buildDir
  try {
    if (-not (Test-Path 'CMakeCache.txt')) {
      Write-Info "Configuring CMake project (Visual Studio 2022 x64)"
      cmake .. -G "Visual Studio 17 2022" -A x64
    } else {
      Write-Info "CMake cache exists, skipping configure"
    }
  } finally { Pop-Location }
}

function Invoke-CMake-Build([string]$buildDir, [string]$config) {
  Write-Info "Building backend ($config)"
  Push-Location $buildDir
  try {
    cmake --build . --config $config
  } finally { Pop-Location }
}

function Resolve-ExePath([string]$buildDir, [string]$config, [string]$exeName) {
  $candidate1 = Join-Path $buildDir (Join-Path $config $exeName)
  $candidate2 = Join-Path $buildDir $exeName
  if (Test-Path -LiteralPath $candidate1) { return (Resolve-Path $candidate1).Path }
  if (Test-Path -LiteralPath $candidate2) { return (Resolve-Path $candidate2).Path }
  return $null
}

function Test-PortOpen([int]$port) {
  try {
    $tcp = New-Object System.Net.Sockets.TcpClient
    $ar = $tcp.BeginConnect('127.0.0.1', $port, $null, $null)
    $success = $ar.AsyncWaitHandle.WaitOne(300)
    $tcp.Close()
    return $success
  } catch { return $false }
}

function Wait-HttpHealthy([string]$url, [int]$timeoutSec = 60) {
  $sw = [Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
    try {
      $res = Invoke-WebRequest -Uri $url -Method GET -TimeoutSec 5 -UseBasicParsing
      if ($res.StatusCode -ge 200 -and $res.StatusCode -lt 500) { return $true }
    } catch { Start-Sleep -Milliseconds 500 }
  }
  return $false
}

function Wait-Url([string]$url, [int]$timeoutSec = 120) {
  $sw = [Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
    try {
      $res = Invoke-WebRequest -Uri $url -Method GET -TimeoutSec 5 -UseBasicParsing
      if ($res.StatusCode -ge 200 -and $res.StatusCode -lt 500) { return $true }
    } catch { Start-Sleep -Milliseconds 500 }
  }
  return $false
}

function Start-Backend([string]$config, [int]$port) {
  $buildDir = Join-Path $PSScriptRoot 'backend/build'
  Ensure-Dir $buildDir
  Invoke-CMake-Configure $buildDir
  Invoke-CMake-Build $buildDir $config

  $exe = Resolve-ExePath $buildDir $config 'os_simulator.exe'
  if (-not $exe) { throw "Backend executable not found: os_simulator.exe (check build output)" }

  if (Test-PortOpen $port) { Write-Warn "Port $port seems busy; another backend may be running. Continuing..." }

  Write-Info "Starting backend: $exe"
  $p = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden
  $pidFile = Join-Path $buildDir 'os_server.pid'
  Set-Content -LiteralPath $pidFile -Value $p.Id -Encoding ascii

  $healthUrl = "http://127.0.0.1:$port/health"
  if (-not (Wait-HttpHealthy $healthUrl 60)) {
    Write-Err "Backend health check failed: $healthUrl"
    throw "Backend did not become healthy in time"
  }
  Write-Info "Backend is ready: $healthUrl (PID=$($p.Id))"
}

function Start-Frontend([string]$mode) {
  $feDir = Join-Path $PSScriptRoot 'frontend'
  if (-not (Test-Path -LiteralPath $feDir)) { throw "Frontend directory 'frontend' not found" }
  Push-Location $feDir
  try {
    Write-Info 'Installing frontend dependencies (npm install)'
    npm install --no-audit --no-fund

    if ($mode -eq 'dev') {
      Write-Info 'Starting frontend dev server: npm run dev -- --host'
      $proc = Start-Process -FilePath 'cmd.exe' -ArgumentList '/c','npm.cmd run dev -- --host' -PassThru -WindowStyle Hidden
      Write-Info "Frontend dev server PID=$($proc.Id) (http://localhost:5173)"
      if (Wait-Url 'http://127.0.0.1:5173' 120) {
        Start-Process -FilePath 'cmd.exe' -ArgumentList '/c start "" http://localhost:5173'
      } else {
        Write-Warn 'Frontend dev server did not become ready in time.'
      }
    } else {
      Write-Info 'Building frontend: npm run build'
      npm run build
      Write-Info 'Starting frontend preview server: npm run preview -- --host'
      $proc = Start-Process -FilePath 'cmd.exe' -ArgumentList '/c','npm.cmd run preview -- --host' -PassThru -WindowStyle Hidden
      Write-Info "Frontend preview server PID=$($proc.Id) (http://localhost:4173 or 5173)"
      if (Wait-Url 'http://127.0.0.1:4173' 120) {
        Start-Process -FilePath 'cmd.exe' -ArgumentList '/c start "" http://localhost:4173'
      } else {
        Write-Warn 'Frontend preview server did not become ready in time.'
      }
    }
  } finally { Pop-Location }
}

# --- Orchestrate ---
Write-Info "Bootstrap: backend($BackendConfig) + frontend($FrontendMode)"
Start-Backend -config $BackendConfig -port $BackendPort
Start-Frontend -mode $FrontendMode

Write-Host "`nBackend API: http://localhost:$BackendPort" -ForegroundColor Green
Write-Host "Frontend  : dev http://localhost:5173; preview http://localhost:4173" -ForegroundColor Green
Write-Host "Note: both processes run in background; you can stop them via Task Manager or by PID." -ForegroundColor Yellow


