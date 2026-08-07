#!/usr/bin/env pwsh
# Simple build script for lusakey Windows

$ErrorActionPreference = "Stop"
$projectDir = "c:\Users\MECHREVO\Documents\GitHub\lusakey"
$buildDir = Join-Path $projectDir "build\windows-x64-release"

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
    
    # Create a batch file to run cmake with proper environment
    $batchFile = Join-Path $buildDir "run-cmake.bat"
    $batchContent = @"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
cmake ..\.. -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=Release -DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
"@
    $batchContent | Set-Content $batchFile -Encoding Ascii
    
    # Run the batch file
    $result = Start-Process cmd -ArgumentList "/c `"$batchFile`"" -Wait -PassThru -NoNewWindow
    
    if ($result.ExitCode -ne 0) {
        Write-Error "CMake configuration failed with exit code $($result.ExitCode)"
        exit $result.ExitCode
    }
    
    Write-Host "CMake configured successfully!" -ForegroundColor Green
    
    # Build
    Write-Host "Building..." -ForegroundColor Cyan
    
    $buildBatchFile = Join-Path $buildDir "run-build.bat"
    $buildBatchContent = @"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
cmake --build . --config Release
"@
    $buildBatchContent | Set-Content $buildBatchFile -Encoding Ascii
    
    $buildResult = Start-Process cmd -ArgumentList "/c `"$buildBatchFile`"" -Wait -PassThru -NoNewWindow
    
    if ($buildResult.ExitCode -ne 0) {
        Write-Error "Build failed with exit code $($buildResult.ExitCode)"
        exit $buildResult.ExitCode
    }
    
    Write-Host "Build completed successfully!" -ForegroundColor Green
    
    # Find the executable
    $exePath = Join-Path $buildDir "app" "lusakey.exe"
    if (Test-Path $exePath) {
        Write-Host "Executable found at: $exePath" -ForegroundColor Green
        
        # Run the executable
        Write-Host "Running the executable..." -ForegroundColor Cyan
        Start-Process $exePath
    } else {
        Write-Warning "Executable not found at expected location: $exePath"
        
        # Try to find it
        $exeFiles = Get-ChildItem -Path $buildDir -Recurse -Filter "lusakey.exe" -ErrorAction SilentlyContinue
        if ($exeFiles.Count -gt 0) {
            Write-Host "Executable found at: $($exeFiles[0].FullName)" -ForegroundColor Green
            Start-Process $exeFiles[0].FullName
        } else {
            Write-Error "Executable not found in build directory"
        }
    }
}
finally {
    Pop-Location
}
