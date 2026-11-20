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
$env:PATH = @(
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.cmake.*" "cmake"),
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.ninja.*" "ninja"),
    (Find-Tool "com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*" "arm-none-eabi-gcc")
) -join ";" + ";" + $env:PATH

# CMake prints normal messages on stderr; Windows PowerShell would treat them as errors under "Stop".
$ErrorActionPreference = "Continue"
Push-Location $PSScriptRoot
try {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    Write-Host "Built build\$Preset\firmware.elf"
} finally {
    Pop-Location
}
