<#
.SYNOPSIS
    Устанавливает тулчейн, необходимый для сборки lusakey на Windows:
    Git, CMake, Ninja, MSVC (Visual Studio Build Tools 2022, C++ workload),
    vcpkg и Qt6 (через aqtinstall).

.DESCRIPTION
    Идемпотентен: перед каждым шагом проверяет, не установлено ли это уже,
    и пропускает шаг, если да. Можно запускать повторно без побочных
    эффектов. Ничего не ставится автоматически при импорте/чтении файла —
    только при явном запуске скрипта.

    После успешного прогона можно собирать проект:
        cmake --preset windows-x64
        cmake --build --preset windows-x64
        ctest --preset windows-x64

    Либо сразу с GUI (Qt6 уже будет в PATH/CMAKE_PREFIX_PATH после
    -IncludeQt и перезапуска терминала):
        cmake --preset windows-x64 -DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH="<QtDir>\<version>\msvc2022_64"
        cmake --build --preset windows-x64

.PARAMETER VcpkgDir
    Куда клонировать vcpkg. По умолчанию C:\dev\vcpkg.

.PARAMETER QtDir
    Куда ставить Qt6 (структура как у официального Qt-инсталлятора). По
    умолчанию C:\Qt.

.PARAMETER QtVersion
    Версия Qt для aqtinstall. По умолчанию 6.7.3 (LTS-ветка, минимум для
    этого проекта — 6.5, см. AGENTS.md).

.PARAMETER SkipQt
    Пропустить установку Qt6 (например, если нужно собрать только ядро
    без GUI — LUSAKEY_BUILD_APP=OFF по умолчанию и так не требует Qt).

.EXAMPLE
    .\setup-toolchain.ps1
    .\setup-toolchain.ps1 -SkipQt
    .\setup-toolchain.ps1 -VcpkgDir D:\tools\vcpkg -QtDir D:\Qt -QtVersion 6.8.0
#>

[CmdletBinding()]
param(
    [string]$VcpkgDir = "C:\dev\vcpkg",
    [string]$QtDir = "C:\Qt",
    [string]$QtVersion = "6.7.3",
    [switch]$SkipQt
)

$ErrorActionPreference = "Stop"

function Write-Step($message) {
    Write-Host ""
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Test-Command($name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

function Install-WingetPackage($id, $overrideArgs) {
    $installed = winget list --id $id --source winget 2>$null | Select-String -Pattern ([regex]::Escape($id))
    if ($installed) {
        Write-Host "  $id уже установлен, пропускаю."
        return
    }
    Write-Host "  Устанавливаю $id ..."
    $args = @('install', '--id', $id, '-e', '--source', 'winget', '--accept-package-agreements', '--accept-source-agreements', '--silent')
    if ($overrideArgs) {
        $args += @('--override', $overrideArgs)
    }
    winget @args
}

function Update-SessionPath {
    $machinePath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    $userPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$machinePath;$userPath"
}

Write-Step "Проверка winget"
if (-not (Test-Command "winget")) {
    throw "winget не найден. Установите App Installer из Microsoft Store и запустите скрипт заново."
}
winget --version

Write-Step "Git, CMake, Ninja"
Install-WingetPackage "Git.Git"
Install-WingetPackage "Kitware.CMake"
Install-WingetPackage "Ninja-build.Ninja"
Update-SessionPath

Write-Step "Visual Studio 2022 Build Tools (C++ workload) — самый долгий шаг, несколько ГБ"
Install-WingetPackage "Microsoft.VisualStudio.2022.BuildTools" `
    "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended"

Write-Step "vcpkg -> $VcpkgDir"
if (Test-Path (Join-Path $VcpkgDir "vcpkg.exe")) {
    Write-Host "  vcpkg уже забутстрапен, пропускаю."
} else {
    Update-SessionPath
    if (-not (Test-Path $VcpkgDir)) {
        git clone https://github.com/microsoft/vcpkg.git $VcpkgDir
    }
    Push-Location $VcpkgDir
    try {
        & .\bootstrap-vcpkg.bat -disableMetrics
    } finally {
        Pop-Location
    }
}

Write-Step "Переменная окружения VCPKG_ROOT"
[System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgDir, "User")
$env:VCPKG_ROOT = $VcpkgDir
Write-Host "  VCPKG_ROOT = $VcpkgDir (сохранено для пользователя; текущая сессия PowerShell обновлена)"

if (-not $SkipQt) {
    Write-Step "Qt $QtVersion -> $QtDir (через aqtinstall)"
    Update-SessionPath
    if (-not (Test-Command "python")) {
        throw "python не найден в PATH — установите Python 3 (например, winget install Python.Python.3.12) и запустите скрипт заново, либо передайте -SkipQt и поставьте Qt отдельно (например, через официальный онлайн-инсталлятор)."
    }

    python -m pip install --quiet --upgrade aqtinstall

    $qtInstallDir = Join-Path $QtDir $QtVersion
    $qtArch = "win64_msvc2019_64"
    if (Test-Path (Join-Path $qtInstallDir "msvc2022_64")) {
        Write-Host "  Qt $QtVersion уже установлен в $qtInstallDir, пропускаю."
    } else {
        # aqtinstall аргументы: install-qt <host> <target> <version> <arch>
        python -m aqt install-qt windows desktop $QtVersion $qtArch --outputdir $QtDir --modules qtquick3d qtshadertools 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "aqt install-qt завершился с ошибкой (см. вывод выше) — попробуйте вручную: python -m aqt list-qt windows desktop, чтобы увидеть доступные версии/архитектуры для вашего Qt."
        }
    }

    Write-Host ""
    Write-Host "  Не забудьте передать CMAKE_PREFIX_PATH при конфигурации GUI, например:"
    Write-Host "    cmake --preset windows-x64 -DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH=`"$qtInstallDir\msvc2022_64`""
} else {
    Write-Step "Qt6 пропущен (-SkipQt) — сборка GUI (LUSAKEY_BUILD_APP=ON) будет недоступна, пока Qt не поставится отдельно."
}

Write-Step "Готово"
Write-Host "Перезапустите терминал (или откройте новый), чтобы подхватить все переменные окружения (PATH, VCPKG_ROOT), затем:"
Write-Host "  cd `"$PSScriptRoot\..\..`""
Write-Host "  cmake --preset windows-x64"
Write-Host "  cmake --build --preset windows-x64"
Write-Host "  ctest --preset windows-x64"

