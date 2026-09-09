# AMBEd Windows service test checklist

This batch adds:

- Timestamped file logging while preserving normal console output.
- 5 MiB startup log rotation (`ambed.log` -> `ambed.log.1`).
- Graceful Ctrl+C / console-close shutdown.
- Graceful SIGINT/SIGTERM shutdown on Linux.
- Native Windows SCM service mode.
- Native `--install` / `--uninstall` helpers.
- Automatic (Delayed Start) service configuration.
- Automatic restart after the first two unexpected service failures.
- Fast server-thread shutdown instead of waiting up to 10 seconds.

## Console test

Build normally, then:

```powershell
.\build\windows-x64\Release\ambed.exe 10.86.10.4
```

Expected:

- Existing AMBEd startup/device output.
- `Log file: ambed.log`
- `Press Ctrl+C to stop AMBEd gracefully`
- Live calls remain smooth.
- Ctrl+C produces `Shutdown requested`, `Stopping AMBEd`, `AMBEd stopped`.
- `ambed.log` contains timestamped copies of the console output.

Optional log path:

```powershell
.\build\windows-x64\Release\ambed.exe 10.86.10.4 --log C:\Temp\ambed-test.log
```

## Service install test

Run an elevated Developer PowerShell:

```powershell
.\build\windows-x64\Release\ambed.exe --install 10.86.10.4
sc.exe start AMBEd
sc.exe query AMBEd
```

Default service log:

```text
C:\ProgramData\AMBEd\ambed.log
```

Then make a real XLX call and verify audio.

Graceful service stop:

```powershell
sc.exe stop AMBEd
sc.exe query AMBEd
```

The log should end with:

```text
Stopping AMBEd
AMBEd stopped
```

Remove the service:

```powershell
.\build\windows-x64\Release\ambed.exe --uninstall
```

Important: for the final release, install the service from its permanent release directory, not from a temporary build directory.
