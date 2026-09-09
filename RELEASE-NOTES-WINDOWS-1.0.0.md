# AMBEd Windows x64 Port v1.0.0

This is the first native Windows x64 release of AMBEd 1.3.5.

## Highlights

- Native MSVC/CMake Windows x64 build
- Native FTDI D2XX access to supported USB AMBE vocoders
- Native Windows SCM service
- Built-in service install/uninstall commands
- Automatic (Delayed Start) service mode
- Service restart policy after unexpected failures
- Timestamped file logging
- 5 MiB startup log rotation
- Graceful Ctrl+C shutdown
- Graceful Windows service stop/shutdown handling
- Windows timer-resolution fix for smooth real-time transcoding
- Modernized Windows socket handling using Winsock2
- Modern IPv4 parsing/name resolution using `inet_pton`, `getaddrinfo`, and `inet_ntop`

## Validation

The release candidate was validated with real FTDI-connected AMBE hardware and live XLX transcoding traffic. Console mode and Windows service mode both completed live streams with zero packet loss in the tested calls, and audio was smooth after the Windows timer-resolution fix.

## FTDI runtime

The FTDI D2XX runtime is not included in the release ZIP. Install the official FTDI Windows driver/runtime separately:

https://ftdichip.com/drivers/d2xx-drivers/

See `README-WINDOWS.md` and `THIRD-PARTY-NOTICES.txt`.
