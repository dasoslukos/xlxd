#include <iostream>
#include <vector>
#include <iomanip>

#include "ftd2xx.h"

int main()
{
    DWORD deviceCount = 0;

    FT_STATUS status = FT_CreateDeviceInfoList(&deviceCount);

    if (status != FT_OK)
    {
        std::cerr
            << "FT_CreateDeviceInfoList failed. FT_STATUS = "
            << status
            << std::endl;

        return 1;
    }

    std::cout
        << "Detected "
        << deviceCount
        << " FTDI device(s)"
        << std::endl
        << std::endl;

    if (deviceCount == 0)
        return 0;

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices(deviceCount);

    status = FT_GetDeviceInfoList(devices.data(), &deviceCount);

    if (status != FT_OK)
    {
        std::cerr
            << "FT_GetDeviceInfoList failed. FT_STATUS = "
            << status
            << std::endl;

        return 1;
    }

    for (DWORD i = 0; i < deviceCount; ++i)
    {
        const auto& dev = devices[i];

        WORD vid = LOWORD(dev.ID);
        WORD pid = HIWORD(dev.ID);

        std::cout << "[" << i << "]" << std::endl;

        std::cout
            << "  Description : "
            << dev.Description
            << std::endl;

        std::cout
            << "  Serial      : "
            << dev.SerialNumber
            << std::endl;

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
            << dev.Type
            << std::endl;

        std::cout
            << "  Flags       : 0x"
            << std::hex
            << dev.Flags
            << std::dec
            << std::endl;

        std::cout << std::endl;
    }

    return 0;
}