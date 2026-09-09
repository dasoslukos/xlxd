#include <iostream>
#include <vector>
#include <iomanip>
#include <string>

#include "ftd2xx.h"

static bool Check(const char* operation, FT_STATUS status)
{
    if (status == FT_OK)
    {
        std::cout << "    " << operation << ": OK" << std::endl;
        return true;
    }

    std::cerr
        << "    " << operation
        << ": FAILED (FT_STATUS=" << status << ")"
        << std::endl;

    return false;
}

static bool TestDevice(const FT_DEVICE_LIST_INFO_NODE& dev)
{
    const std::string description(dev.Description);
    const std::string serial(dev.SerialNumber);

    DWORD baudRate = 0;

    if (description == "DVstick-33")
        baudRate = 921600;
    else if (description == "USB-3000")
        baudRate = 460800;
    else
    {
        std::cout
            << "Skipping unsupported device: "
            << description
            << std::endl;

        return true;
    }

    std::cout << std::endl;
    std::cout
        << "Opening "
        << description
        << " (" << serial << ")"
        << std::endl;

    FT_HANDLE handle = nullptr;

    FT_STATUS status = FT_OpenEx(
        (PVOID)dev.SerialNumber,
        FT_OPEN_BY_SERIAL_NUMBER,
        &handle
    );

    if (!Check("FT_OpenEx", status))
        return false;

    bool ok = true;

    Sleep(50);

    ok &= Check(
        "FT_Purge",
        FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX)
    );

    Sleep(50);

    ok &= Check(
        "FT_SetDataCharacteristics",
        FT_SetDataCharacteristics(
            handle,
            FT_BITS_8,
            FT_STOP_BITS_1,
            FT_PARITY_NONE
        )
    );

    ok &= Check(
        "FT_SetFlowControl",
        FT_SetFlowControl(
            handle,
            FT_FLOW_RTS_CTS,
            0x11,
            0x13
        )
    );

    ok &= Check(
        "FT_SetRts",
        FT_SetRts(handle)
    );

    ok &= Check(
        "FT_ClrDtr",
        FT_ClrDtr(handle)
    );

    std::cout
        << "    Baud rate: "
        << baudRate
        << std::endl;

    ok &= Check(
        "FT_SetBaudRate",
        FT_SetBaudRate(handle, baudRate)
    );

    ok &= Check(
        "FT_SetLatencyTimer",
        FT_SetLatencyTimer(handle, 4)
    );

    ok &= Check(
        "FT_SetUSBParameters",
        FT_SetUSBParameters(handle, 1024, 0)
    );

    ok &= Check(
        "FT_SetTimeouts",
        FT_SetTimeouts(handle, 200, 200)
    );

    status = FT_Close(handle);

    if (!Check("FT_Close", status))
        ok = false;

    std::cout
        << (ok ? "  Device test PASSED" : "  Device test FAILED")
        << std::endl;

    return ok;
}

int main()
{
    DWORD deviceCount = 0;

    FT_STATUS status =
        FT_CreateDeviceInfoList(&deviceCount);

    if (!Check("FT_CreateDeviceInfoList", status))
        return 1;

    std::cout
        << std::endl
        << "Detected "
        << deviceCount
        << " FTDI device(s)"
        << std::endl;

    if (deviceCount == 0)
        return 0;

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices(deviceCount);

    status =
        FT_GetDeviceInfoList(devices.data(), &deviceCount);

    if (!Check("FT_GetDeviceInfoList", status))
        return 1;

    bool allOK = true;

    for (DWORD i = 0; i < deviceCount; ++i)
    {
        const auto& dev = devices[i];

        WORD vid = HIWORD(dev.ID);
        WORD pid = LOWORD(dev.ID);

        std::cout << std::endl;
        std::cout << "[" << i << "]" << std::endl;

        std::cout
            << "  Description : "
            << dev.Description << std::endl;

        std::cout
            << "  Serial      : "
            << dev.SerialNumber << std::endl;

        std::cout
            << "  VID:PID     : "
            << std::hex
            << std::uppercase
            << std::setfill('0')
            << std::setw(4) << vid
            << ":"
            << std::setw(4) << pid
            << std::dec
            << std::endl;

        std::cout
            << "  Type        : "
            << dev.Type << std::endl;

        std::cout
            << "  Flags       : 0x"
            << std::hex << dev.Flags
            << std::dec << std::endl;

        if (!TestDevice(dev))
            allOK = false;
    }

    std::cout << std::endl;

    if (allOK)
    {
        std::cout
            << "All supported AMBE devices opened and configured successfully."
            << std::endl;

        return 0;
    }

    std::cerr
        << "One or more device tests failed."
        << std::endl;

    return 1;
}