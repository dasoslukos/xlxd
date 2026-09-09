# AMBEd for Windows x64

This package contains the native Windows x64 port of AMBEd 1.3.5, the hardware transcoder daemon used with XLX.

## What is included

- Native Windows x64 `ambed.exe`
- Console mode
- Native Windows Service Control Manager (SCM) support
- Timestamped file logging
- Graceful Ctrl+C and service shutdown
- Automatic (Delayed Start) service configuration
- Automatic restart after unexpected service failures
- Windows timer-resolution handling required for smooth real-time transcoding

## Requirements

- Windows 10 or Windows 11 x64
- Supported AMBE hardware connected through FTDI USB
- FTDI D2XX driver/runtime installed
- Network connectivity between AMBEd and the XLX reflector
- UDP 10100-10199 allowed through the Windows firewall/NAT as required by your topology

### FTDI D2XX driver

The FTDI D2XX runtime is **not bundled in this release package**.

Windows can normally obtain the FTDI driver through Windows Update after an FTDI-based device is connected. FTDI also provides an official Windows installer containing both VCP and D2XX support:

https://ftdichip.com/drivers/d2xx-drivers/

Install the official FTDI driver before running AMBEd if `ftd2xx.dll` is not already present on the system.

## Console mode

From PowerShell:

```powershell
.\ambed.exe 192.168.1.50
```

Replace `192.168.1.50` with the IPv4 address AMBEd should bind to.

The default console-mode log is:

```text
ambed.log
```

To specify another log file:

```powershell
.\ambed.exe 192.168.1.50 --log C:\Logs\ambed.log
```

Press **Ctrl+C** to stop AMBEd gracefully.

## Windows service mode

**Important:** extract/copy the release to its permanent location before installing the service. The service stores the full path to the current `ambed.exe`.

Open an elevated PowerShell or Command Prompt.

Install:

```powershell
.\ambed.exe --install 192.168.1.50
```

The service is installed as:

```text
Service name: AMBEd
Display name: AMBEd Transcoder
Startup: Automatic (Delayed Start)
Account: LocalSystem
```

Start it:

```powershell
sc.exe start AMBEd
```

Check status:

```powershell
sc.exe query AMBEd
```

Stop it gracefully:

```powershell
sc.exe stop AMBEd
```

Remove the service:

```powershell
.\ambed.exe --uninstall
```

The default service log is:

```text
C:\ProgramData\AMBEd\ambed.log
```

To install with a custom service log:

```powershell
.\ambed.exe --install 192.168.1.50 --log D:\Logs\ambed.log
```

## Firewall

AMBEd uses UDP port 10100 for control and UDP 10101-10199 for transcoder streams.

Example inbound Windows Firewall rule from an elevated PowerShell:

```powershell
New-NetFirewallRule `
    -DisplayName "AMBEd UDP 10100-10199" `
    -Direction Inbound `
    -Protocol UDP `
    -LocalPort 10100-10199 `
    -Action Allow
```

If AMBEd is behind NAT, forward UDP 10100-10199 to the Windows host.

After changing NAT rules, stale UDP firewall/NAT states may continue pointing at the previous host until those states expire or are cleared.

## Logging

Console mode mirrors output to both the console and the log file.

Service mode writes to the log file only.

Log lines include local timestamps and severity:

```text
2026-09-09 14:31:55.639 [INFO] Stream Open from XLX...
```

When a log is at least 5 MiB at startup, AMBEd rotates it to:

```text
ambed.log.1
```

## Real-time timing on Windows

AMBEd's vocoder loop uses millisecond-scale sleeps. This Windows port requests 1 ms timer resolution while AMBEd is running. Without that adjustment, normal Windows timer granularity can turn the original 2 ms processing delay into much longer waits and produce severely choppy audio.

The timer period is restored during graceful shutdown.

## Command-line reference

```text
ambed <ip> [--log <path>]
ambed --install <ip> [--log <path>]
ambed --uninstall
ambed --version
ambed --help
```

`--service` is an internal mode used by the Windows Service Control Manager and normally should not be started manually.

## Troubleshooting

### AMBEd starts but receives no reflector streams

Check:

- Windows Firewall allows UDP 10100-10199
- NAT points to the current Windows AMBEd host
- old UDP states were cleared after changing the NAT target
- the reflector is sending to the correct public IP/port range

### AMBEd cannot see the USB vocoders

Check Device Manager and confirm the FTDI devices are present and healthy. Install/update the official FTDI D2XX driver if needed.

### Service will not start

Check:

```powershell
sc.exe query AMBEd
Get-Content C:\ProgramData\AMBEd\ambed.log -Tail 100
```

Also verify that the service's executable path still exists:

```powershell
sc.exe qc AMBEd
```

If the release directory was moved after installation, uninstall and reinstall the service from the new permanent location.

## License and source

AMBEd / XLXD source is distributed under the GNU General Public License version 3. See `license.txt`.

Source repository:

https://github.com/dasoslukos/xlxd

Windows-port branch:

```text
ambed-windows-x64
```

FTDI D2XX is proprietary third-party software and is not included in this Windows release package. See `THIRD-PARTY-NOTICES.txt`.
