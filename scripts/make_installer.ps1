$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$installerDir = Join-Path $root "installer"

if (-not (Get-Command iscc -ErrorAction SilentlyContinue)) {
  Write-Error "Inno Setup Compiler (iscc) not found. Install Inno Setup first."
}

Push-Location $installerDir
iscc "LusaKey.iss"
Pop-Location

Write-Host "Installer created in installer/ as LusaKeySetup.exe."
