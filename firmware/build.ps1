# Builds the DitDah firmware with the CMake, Ninja and GCC that ship inside STM32CubeIDE.
# Usage (from this folder):  .\build.ps1            -> build\Debug\firmware.elf
#                            .\build.ps1 Release    -> build\Release\firmware.elf
param(
    [ValidateSet("Debug", "Release")] [string] $Preset = "Debug",
    [string] $CubeIde = $env:STM32CUBEIDE_DIR
)
$ErrorActionPreference = "Stop"

if (-not $CubeIde) {
    # Look for an STM32CubeIDE_x.y.z folder in C:\ST or up to one folder deep on any drive.
    $roots = @("C:\ST") + (Get-PSDrive -PSProvider FileSystem | ForEach-Object { $_.Root })
    $CubeIde = $roots | Where-Object { Test-Path $_ } |
        ForEach-Object { Get-ChildItem $_ -Directory -Depth 1 -Filter "STM32CubeIDE*" -ErrorAction SilentlyContinue } |
        ForEach-Object { Join-Path $_.FullName "STM32CubeIDE" } |
        Where-Object { Test-Path (Join-Path $_ "plugins") } | Select-Object -First 1
}
if (-not $CubeIde) { throw "Can't find STM32CubeIDE. Set STM32CUBEIDE_DIR to the folder that contains 'plugins'." }

$plugins = Join-Path $CubeIde "plugins"
function Find-Tool($pattern, $exe) {
    $dir = Get-ChildItem $plugins -Directory -Filter $pattern | Sort-Object Name -Descending | Select-Object -First 1
    if (-not $dir) { throw "Can't find $exe in $plugins" }
    return Join-Path $dir.FullName "tools\bin"
}
$toolDirs = @(
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.cmake.*" "cmake"),
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.ninja.*" "ninja"),
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*" "arm-none-eabi-gcc")
)
$env:PATH = ($toolDirs -join ";") + ";" + $env:PATH

# CubeMX overwrites the USB HID class with its mouse version every time it generates code.
$hidC = Join-Path $PSScriptRoot "Middlewares\ST\STM32_USB_Device_Library\Class\HID\Src\usbd_hid.c"
if (-not (Select-String -Path $hidC -Pattern "DITDAH_KEYBOARD_PATCH" -Quiet)) {
    throw "The USB HID files are the CubeMX mouse version (CubeMX regenerated them). Run .\tools\patch-hid-keyboard.ps1, then build again."
}

# CMake prints normal messages (such as "Build type: Debug") on stderr. Windows PowerShell turns each
# stderr line into an error record, so a caller (or Kiro) sees an error even when the build succeeds.
# Merge stderr into normal output as plain text and judge success by the exit code alone.
$ErrorActionPreference = "Continue"
Push-Location $PSScriptRoot
try {
    cmake --preset $Preset 2>&1 | ForEach-Object { "$_" }
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    cmake --build --preset $Preset 2>&1 | ForEach-Object { "$_" }
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    Write-Host "Built build\$Preset\firmware.elf"
} finally {
    Pop-Location
}
