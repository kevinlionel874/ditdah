# Shows the Blue Pill's debug log from the CP2102 (USART1, 115200 baud). Press Ctrl+C to stop.
# Usage:  .\monitor.ps1            (finds the CP2102 by itself)
#         .\monitor.ps1 -Port COM5
param(
    [string] $Port,
    [int] $Baud = 115200
)

if (-not $Port) {
    $dev = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match "CP210x.*\(COM\d+\)" } | Select-Object -First 1
    if ($dev -and ($dev.Name -match "\((COM\d+)\)")) { $Port = $Matches[1] }
}
if (-not $Port) {
    throw "No CP2102 found. Plug it in and install the CP210x driver, or pass the port: .\monitor.ps1 -Port COM5"
}

$serial = New-Object System.IO.Ports.SerialPort $Port, $Baud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$serial.ReadTimeout = 500
$serial.NewLine = "`n"
$serial.Open()
Write-Host "Listening on $Port at $Baud baud. Press the Blue Pill's RESET button to see the start-up messages. Ctrl+C to stop."
try {
    while ($true) {
        try {
            $line = $serial.ReadLine()
            Write-Host $line.TrimEnd("`r")
        } catch [System.TimeoutException] {
        }
    }
} finally {
    $serial.Close()
}
