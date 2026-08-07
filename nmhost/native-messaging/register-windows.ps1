<#
.SYNOPSIS
    Регистрирует lusakey-nmhost.exe как native-messaging host для Chrome и
    Firefox на Windows.

.DESCRIPTION
    Идемпотентен: можно запускать повторно (например, после пересборки —
    путь к exe и содержимое манифеста просто перезаписываются). Ничего не
    ставится/не регистрируется при импорте/чтении файла — только при явном
    запуске скрипта.

    После регистрации загрузите расширение из browser-extension/ как
    unpacked-расширение в Chrome/Edge и скопируйте его ID в параметр
    -ExtensionId (обычно это 32-символьная строка на странице
    chrome://extensions в режиме разработчика).

.PARAMETER ExePath
    Путь к собранному lusakey-nmhost.exe. По умолчанию — стандартное место
    сборки (build\windows-x64\nmhost\lusakey-nmhost.exe относительно корня
    репозитория).

.PARAMETER ExtensionId
    Id расширения Chrome, которому разрешено говорить с хостом
    (подставляется в allowed_origins манифеста). Обязателен для рабочего
    расширения. По умолчанию — плейсхолдер (расширение не подключится).

.EXAMPLE
    .\register-windows.ps1 -ExtensionId "abcdefghijklmnopqrstuvwxabcdefgh"
#>

[CmdletBinding()]
param(
    [string]$ExePath = (Join-Path $PSScriptRoot "..\..\build\windows-x64\nmhost\lusakey-nmhost.exe"),
    [string]$ExtensionId = "REPLACE_WITH_REAL_EXTENSION_ID"
)

$ErrorActionPreference = "Stop"

function Write-Step($message) {
    Write-Host ""
    Write-Host "==> $message" -ForegroundColor Cyan
}

$resolvedExePath = Resolve-Path $ExePath -ErrorAction SilentlyContinue
if (-not $resolvedExePath) {
    throw "Не найден $ExePath — сначала соберите: cmake --preset windows-x64 -DLUSAKEY_BUILD_NMHOST=ON, затем cmake --build --preset windows-x64"
}
$resolvedExePath = $resolvedExePath.Path

Write-Step "Рендерю манифест для $resolvedExePath"
$manifestTemplate = Get-Content (Join-Path $PSScriptRoot "com.lusakey.nmhost.json") -Raw
$manifestJson = $manifestTemplate `
    -replace 'REPLACED_AT_INSTALL_TIME', ($resolvedExePath -replace '\\', '\\\\') `
    -replace 'REPLACE_WITH_REAL_EXTENSION_ID', $ExtensionId

$manifestDir = Join-Path $env:LOCALAPPDATA "lusakey\native-messaging"
New-Item -ItemType Directory -Force -Path $manifestDir | Out-Null
$manifestPath = Join-Path $manifestDir "com.lusakey.nmhost.json"
Set-Content -Path $manifestPath -Value $manifestJson -Encoding UTF8
Write-Host "  Манифест: $manifestPath"

Write-Step "Регистрирую в реестре (Chrome + Firefox)"
$chromeKey = "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.lusakey.nmhost"
New-Item -Path $chromeKey -Force | Out-Null
Set-ItemProperty -Path $chromeKey -Name "(Default)" -Value $manifestPath
Write-Host "  $chromeKey -> $manifestPath"

$firefoxKey = "HKCU:\Software\Mozilla\NativeMessagingHosts\com.lusakey.nmhost"
New-Item -Path $firefoxKey -Force | Out-Null
Set-ItemProperty -Path $firefoxKey -Name "(Default)" -Value $manifestPath
Write-Host "  $firefoxKey -> $manifestPath"

Write-Step "Готово"
Write-Host "Проверить протокол без браузера — см. README.md в этой папке (раздел про сырой stdin/stdout)."

