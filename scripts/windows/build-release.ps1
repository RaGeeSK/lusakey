<#
.SYNOPSIS
    Собирает lusakey в Release-конфигурации и упаковывает её в портативный
    zip и Inno Setup инсталлятор; опционально публикует оба файла в релиз
    на GitHub.

.DESCRIPTION
    Пайплайн:
      1. cmake --preset windows-x64-release (первый прогон соберёт
         Release-версии зависимостей через vcpkg — дольше debug-сборки).
      2. cmake --build --preset windows-x64-release
      3. Стейджинг в build\windows-x64-release\dist\lusakey: exe + DLL
         зависимостей ядра + шрифты + всё, что достаёт windeployqt (Qt6
         DLL, платформенный плагин, нужные QML-модули, MSVC runtime).
      4. Compress-Archive -> build\installers\lusakey-<version>-win64-portable.zip
      5. iscc packaging\windows\installer.iss -> build\installers\lusakey-<version>-setup.exe
      6. С -PublishRelease: gh release create <tag> с обоими файлами.

    Требует Inno Setup (`iscc`) и, для публикации, GitHub CLI (`gh`, уже
    авторизованный через `gh auth login`) — оба ставятся
    scripts\windows\setup-toolchain.ps1 (без -SkipPackaging).

.PARAMETER Version
    Версия релиза. По умолчанию 0.1.0 — держите в синхроне с AppVersion в
    packaging\windows\installer.iss и CPACK_PACKAGE_VERSION в корневом
    CMakeLists.txt.

.PARAMETER PublishRelease
    Если указан — после сборки создаёт релиз на GitHub (gh release create)
    и заливает туда zip + инсталлятор. Без этого флага скрипт только
    собирает файлы локально в build\installers\.

.PARAMETER QtBinDir
    Путь к bin-папке нужного Qt-кита (для windeployqt.exe).

.EXAMPLE
    .\build-release.ps1
    .\build-release.ps1 -Version 0.2.0 -PublishRelease
#>

[CmdletBinding()]
param(
    [string]$Version = "0.1.0",
    [switch]$PublishRelease,
    [string]$QtBinDir = "C:\Qt\6.7.3\msvc2019_64\bin",
    [string]$VcpkgRoot = "C:\dev\vcpkg",
    [string]$VcvarsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    [string]$IsccPath
)

$ErrorActionPreference = "Continue"

function Write-Step($message) {
    Write-Host ""
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Test-Command($name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$BuildDir = Join-Path $RepoRoot "build\windows-x64-release"
$DistDir = Join-Path $BuildDir "dist\lusakey"
$InstallersDir = Join-Path $RepoRoot "build\installers"

if (-not $IsccPath) {
    $isccCmd = Get-Command "iscc" -ErrorAction SilentlyContinue
    if ($isccCmd) {
        $IsccPath = $isccCmd.Source
    } else {
        $candidates = @(
            "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
            "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
            "C:\Program Files\Inno Setup 6\ISCC.exe"
        )
        $IsccPath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    }
}
if (-not $IsccPath) {
    throw "ISCC.exe (Inno Setup) не найден ни в PATH, ни в стандартных папках установки. Поставьте его через scripts\windows\setup-toolchain.ps1 (без -SkipPackaging), либо передайте -IsccPath явно."
}

Write-Step "Останавливаю запущенный lusakey.exe (если есть)"
Get-Process -Name lusakey -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Step "Импортирую окружение MSVC (vcvars64.bat)"
if (-not (Test-Path $VcvarsPath)) {
    throw "vcvars64.bat не найден по пути $VcvarsPath — передайте -VcvarsPath с правильным расположением Visual Studio Build Tools."
}
$vcvarsOutput = cmd /c "`"$VcvarsPath`" && set"
foreach ($line in $vcvarsOutput) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}
$env:VCPKG_ROOT = $VcpkgRoot

Write-Step "Конфигурирую Release (windows-x64-release)"
Push-Location $RepoRoot
try {
    cmake --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "cmake configure завершился с ошибкой." }

    Write-Step "Собираю Release"
    cmake --build --preset windows-x64-release
    if ($LASTEXITCODE -ne 0) { throw "cmake build завершился с ошибкой." }
} finally {
    Pop-Location
}

Write-Step "Стейджинг портативной сборки в $DistDir"
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

$builtAppDir = Join-Path $BuildDir "app"
Copy-Item (Join-Path $builtAppDir "lusakey.exe") $DistDir
Copy-Item (Join-Path $builtAppDir "libsodium.dll") $DistDir -ErrorAction SilentlyContinue
$fontsSrc = Join-Path $builtAppDir "fonts"
if (Test-Path $fontsSrc) {
    Copy-Item $fontsSrc (Join-Path $DistDir "fonts") -Recurse
}

Write-Step "windeployqt (Qt6 DLL, платформенный плагин, QML-модули, MSVC runtime)"
$windeployqt = Join-Path $QtBinDir "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt.exe не найден в $QtBinDir — передайте -QtBinDir с правильным путём к bin-папке вашего Qt-кита."
}
& $windeployqt --release --no-translations --compiler-runtime --qmldir (Join-Path $RepoRoot "app\ui") (Join-Path $DistDir "lusakey.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt завершился с ошибкой." }

New-Item -ItemType Directory -Force -Path $InstallersDir | Out-Null

Write-Step "Портативный zip"
$zipPath = Join-Path $InstallersDir "lusakey-$Version-win64-portable.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $DistDir "*") -DestinationPath $zipPath
Write-Host "  -> $zipPath"

Write-Step "Инсталлятор (Inno Setup)"
Push-Location (Join-Path $RepoRoot "packaging\windows")
try {
    & $IsccPath "/DAppVersion=$Version" installer.iss
    if ($LASTEXITCODE -ne 0) { throw "iscc завершился с ошибкой." }
} finally {
    Pop-Location
}
$installerPath = Join-Path $InstallersDir "lusakey-$Version-setup.exe"
Write-Host "  -> $installerPath"

if ($PublishRelease) {
    Write-Step "Публикую релиз на GitHub"
    if (-not (Test-Command "gh")) {
        throw "gh (GitHub CLI) не найден в PATH. Поставьте его через scripts\windows\setup-toolchain.ps1 (без -SkipPackaging), затем `gh auth login`."
    }
    $tag = "v$Version"
    Push-Location $RepoRoot
    try {
        gh release create $tag $zipPath $installerPath --title "lusakey $Version" --generate-notes
        if ($LASTEXITCODE -ne 0) { throw "gh release create завершился с ошибкой." }
    } finally {
        Pop-Location
    }
    Write-Host "  Релиз опубликован: $tag"
} else {
    Write-Step "Готово (без публикации)"
    Write-Host "  Для публикации на GitHub: .\build-release.ps1 -Version $Version -PublishRelease"
    Write-Host "  (нужен авторизованный `gh` — один раз выполните `gh auth login`)"
}

