param (
    [string]$Action = "build"
)

$ErrorActionPreference = "Stop"

Write-Host "Setting up ESP-IDF environment..." -ForegroundColor Cyan

# Try to find ESP-IDF export script in common locations
$EspIdfPaths = @(
    "$env:USERPROFILE\esp\esp-idf\export.ps1",
    "$env:USERPROFILE\esp\v6.0.1\esp-idf\export.ps1",
    "C:\Espressif\frameworks\esp-idf-v6.0.1\export.ps1",
    "C:\esp\esp-idf\export.ps1"
)

$ExportScript = $null
foreach ($Path in $EspIdfPaths) {
    if (Test-Path $Path) {
        $ExportScript = $Path
        break
    }
}

if ($null -eq $ExportScript) {
    # If not found in common paths, check if IDF_PATH is set
    if ($env:IDF_PATH -and (Test-Path "$env:IDF_PATH\export.ps1")) {
        $ExportScript = "$env:IDF_PATH\export.ps1"
    } else {
        Write-Host "Error: Could not find ESP-IDF export.ps1. Please ensure ESP-IDF is installed." -ForegroundColor Red
        exit 1
    }
}

Write-Host "Found ESP-IDF at $ExportScript" -ForegroundColor Green
. $ExportScript

if ($Action -eq "build") {
    Write-Host "Building project..." -ForegroundColor Cyan
    idf.py build
} elseif ($Action -eq "flash") {
    Write-Host "Building and flashing project..." -ForegroundColor Cyan
    idf.py build flash monitor
} else {
    Write-Host "Running custom idf.py command: idf.py $Action" -ForegroundColor Cyan
    Invoke-Expression "idf.py $Action"
}
