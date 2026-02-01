$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  Write-Error "cmake not found. Install CMake and a C++ toolchain (MSVC or MinGW) first."
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Push-Location $buildDir
cmake -S $root -B $buildDir
cmake --build $buildDir --config Release
Pop-Location

Write-Host "Build done. Output in build/ (Release configuration)."
