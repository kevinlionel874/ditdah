# Turns the CubeMX USB HID class (a mouse by default) into a boot keyboard.
# CubeMX rewrites these files every time it generates code, so run this again after each
# "GENERATE CODE". It is safe to run twice: it does nothing if the files are already patched.
# Usage (from the firmware folder):  .\tools\patch-hid-keyboard.ps1
$ErrorActionPreference = "Stop"

$firmware = Split-Path -Parent $PSScriptRoot
$hidDir = Join-Path $firmware "Middlewares\ST\STM32_USB_Device_Library\Class\HID"
$hFile = Join-Path $hidDir "Inc\usbd_hid.h"
$cFile = Join-Path $hidDir "Src\usbd_hid.c"
$marker = "DITDAH_KEYBOARD_PATCH"
$utf8 = New-Object System.Text.UTF8Encoding $false

$c = [IO.File]::ReadAllText($cFile)
if ($c.Contains($marker)) {
    Write-Host "usbd_hid.c is already patched."
    exit 0
}
$h = [IO.File]::ReadAllText($hFile)

# 1. 8-byte reports and the new descriptor size (45 bytes, below).
$h2 = $h -replace "(#define\s+HID_EPIN_SIZE\s+)0x04U", '${1}0x08U'
$h2 = $h2 -replace "(#define\s+HID_MOUSE_REPORT_DESC_SIZE\s+)74U", '${1}45U'
if ($h2 -eq $h -or -not ($h2 -match "HID_EPIN_SIZE\s+0x08U") -or -not ($h2 -match "HID_MOUSE_REPORT_DESC_SIZE\s+45U")) {
    throw "usbd_hid.h doesn't look like the CubeMX mouse version; nothing changed."
}

# 2. Interface protocol 2 (mouse) -> 1 (keyboard) in all three configuration descriptors.
$protocolPattern = "0x02,(\s*/\*nInterfaceProtocol)"
$count = ([regex]::Matches($c, $protocolPattern)).Count
if ($count -lt 1) { throw "Couldn't find nInterfaceProtocol in usbd_hid.c; nothing changed." }
$c2 = [regex]::Replace($c, $protocolPattern, '0x01,${1}')

# 3. Boot keyboard report: modifier byte, reserved byte, 6 key codes. Key codes go up to
#    0x73 so that F13 (0x68, "accept the suggestion") is allowed. No LED output report,
#    because the ST HID class doesn't handle SET_REPORT.
$keyboardDesc = @"
  0x05, 0x01,  /* Usage Page (Generic Desktop) */
  0x09, 0x06,  /* Usage (Keyboard) */
  0xA1, 0x01,  /* Collection (Application) */
  0x05, 0x07,  /*   Usage Page (Key Codes) */
  0x19, 0xE0,  /*   Usage Minimum (Left Control) */
  0x29, 0xE7,  /*   Usage Maximum (Right GUI) */
  0x15, 0x00,  /*   Logical Minimum (0) */
  0x25, 0x01,  /*   Logical Maximum (1) */
  0x75, 0x01,  /*   Report Size (1) */
  0x95, 0x08,  /*   Report Count (8) */
  0x81, 0x02,  /*   Input (Data, Variable, Absolute): modifier byte */
  0x95, 0x01,  /*   Report Count (1) */
  0x75, 0x08,  /*   Report Size (8) */
  0x81, 0x01,  /*   Input (Constant): reserved byte */
  0x95, 0x06,  /*   Report Count (6) */
  0x75, 0x08,  /*   Report Size (8) */
  0x15, 0x00,  /*   Logical Minimum (0) */
  0x25, 0x73,  /*   Logical Maximum (0x73 = F24) */
  0x05, 0x07,  /*   Usage Page (Key Codes) */
  0x19, 0x00,  /*   Usage Minimum (0) */
  0x29, 0x73,  /*   Usage Maximum (0x73) */
  0x81, 0x00,  /*   Input (Data, Array): 6 key codes */
  0xC0         /* End Collection */
"@
$arrayPattern = "(?s)(HID_MOUSE_ReportDesc\[HID_MOUSE_REPORT_DESC_SIZE\]\s*__ALIGN_END\s*=\s*\{).*?(\};)"
if (-not ([regex]::IsMatch($c2, $arrayPattern))) { throw "Couldn't find the report descriptor in usbd_hid.c; nothing changed." }
$c2 = [regex]::Replace($c2, $arrayPattern, { param($m) $m.Groups[1].Value + "`n" + $keyboardDesc + "`n" + $m.Groups[2].Value })

$c2 = "/* $marker`: this file was changed from a mouse to a boot keyboard by firmware/tools/patch-hid-keyboard.ps1 */`n" + $c2

[IO.File]::WriteAllText($hFile, $h2, $utf8)
[IO.File]::WriteAllText($cFile, $c2, $utf8)
Write-Host "Patched usbd_hid.h and usbd_hid.c: boot keyboard, 8-byte reports, protocol 1 ($count descriptors)."
