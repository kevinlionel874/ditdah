# Flashes build\<Preset>\firmware.elf to the Blue Pill through an ST-Link (SWD), then resets it.
# Wire the ST-Link first (see docs/cubemx-setup.md). Usage:  .\flash.ps1   or   .\flash.ps1 Release
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

$prog = Get-ChildItem (Join-Path $CubeIde "plugins") -Directory -Filter "com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.*" |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $prog) { throw "Can't find STM32CubeProgrammer inside STM32CubeIDE" }
$cli = Join-Path $prog.FullName "tools\bin\STM32_Programmer_CLI.exe"

$elf = Join-Path $PSScriptRoot "build\$Preset\firmware.elf"
if (-not (Test-Path $elf)) { throw "No $elf yet. Run .\build.ps1 $Preset first." }

$ErrorActionPreference = "Continue"
& $cli -c port=SWD -w $elf -v -rst
if ($LASTEXITCODE -ne 0) { throw "Flashing failed. Check the four ST-Link wires and that the board's BOOT0 jumper is on 0." }
