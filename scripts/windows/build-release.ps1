#!/usr/bin/env pwsh
# Build script for lusakey Windows release
# This script builds the project using Visual Studio Developer Command Prompt

param(
    [switch]$SkipPackaging
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Split-Path -Parent $scriptDir
$buildDir = Join-Path $projectDir "build" "windows-x64-release"

# Clean previous build
if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}

# Create build directory
New-Item -ItemType Directory -Path $buildDir | Out-Null

# Change to build directory
Push-Location $buildDir

try {
    # Run CMake configuration with Visual Studio Developer Command Prompt
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat`" && cmake ..\.. -G Ninja -DCMAKE_BUILD_TYPE=Release -DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH=`"C:/Qt/6.7.3/msvc2019_64`""
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit $LASTEXITCODE
    }
    
    # Build
    Write-Host "Building..." -ForegroundColor Cyan
    cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat`" && cmake --build . --config Release"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit $LASTEXITCODE
    }
    
    Write-Host "Build completed successfully!" -ForegroundColor Green
    
    # Find the executable
    $exePath = Join-Path $buildDir "lusakey.exe"
    if (Test-Path $exePath) {
        Write-Host "Executable found at: $exePath" -ForegroundColor Green
        
        if (!$SkipPackaging) {
            # TODO: Add packaging logic here
            Write-Host "Packaging is not implemented yet" -ForegroundColor Yellow
        }
        
        # Try to run the executable
        Write-Host "Attempting to run the executable..." -ForegroundColor Cyan
        Start-Process $exePath
    } else {
        Write-Warning "Executable not found at expected location"
        # Try to find it
        $exeFiles = Get-ChildItem -Path $buildDir -Recurse -Filter "lusakey.exe" -ErrorAction SilentlyContinue
        if ($exeFiles.Count -gt 0) {
            Write-Host "Executable found at: $($exeFiles[0].FullName)" -ForegroundColor Green
        }
    }
}
finally {
    Pop-Location
}
