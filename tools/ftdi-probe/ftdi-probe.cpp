#include <iostream>
#include <vector>
#include <iomanip>
#include <string>

#include "ftd2xx.h"

static constexpr unsigned char PKT_HEADER     = 0x61;
static constexpr unsigned char PKT_CONTROL    = 0x00;
static constexpr unsigned char PKT_CHANNEL    = 0x01;
static constexpr unsigned char PKT_SPEECH     = 0x02;

static constexpr unsigned char PKT_PRODID     = 0x30;
static constexpr unsigned char PKT_VERSTRING  = 0x31;
static constexpr unsigned char PKT_PARITYBYTE = 0x2F;
static constexpr unsigned char PKT_RESET      = 0x33;
static constexpr unsigned char PKT_READY      = 0x39;

static bool Check(const char* operation, FT_STATUS status)
{
    if (status == FT_OK)
    {
        std::cout
            << "    "
            << operation
            << ": OK"
            << std::endl;

        return true;
    }

    std::cerr
        << "    "
        << operation
        << ": FAILED (FT_STATUS="
        << status
        << ")"
        << std::endl;

    return false;
}

static bool WriteExact(
    FT_HANDLE handle,
    const unsigned char* data,
    DWORD length)
{
    DWORD written = 0;

    FT_STATUS status = FT_Write(
        handle,
        (LPVOID)data,
        length,
        &written
    );

    if (status != FT_OK)
    {
        std::cerr
            << "    FT_Write failed, status="
            << status
            << std::endl;

        return false;
    }

    if (written != length)
    {
        std::cerr
            << "    Short FT_Write: "
            << written
            << "/"
            << length
            << std::endl;

        return false;
    }

    return true;
}

static bool ReadExact(
    FT_HANDLE handle,
    unsigned char* data,
    DWORD length)
{
    DWORD received = 0;

    FT_STATUS status = FT_Read(
        handle,
        data,
        length,
        &received
    );

    if (status != FT_OK)
    {
        std::cerr
            << "    FT_Read failed, status="
            << status
            << std::endl;

        return false;
    }

    if (received != length)
    {
        std::cerr
            << "    Short FT_Read: "
            << received
            << "/"
            << length
            << std::endl;

        return false;
    }

    return true;
}

static int ReadPacket(
    FT_HANDLE handle,
    unsigned char* packet,
    int maxLength)
{
    // Every AMBE packet starts with a four-byte header.
    if (!ReadExact(handle, packet, 4))
        return 0;

    // Validate the framing and packet type.
    if (packet[0] != PKT_HEADER ||
        (packet[3] != PKT_CONTROL &&
         packet[3] != PKT_CHANNEL &&
         packet[3] != PKT_SPEECH))
    {
        std::cerr
            << "    Invalid AMBE packet header"
            << std::endl;

        FT_Purge(handle, FT_PURGE_RX);

        return 0;
    }

    // Bytes 1 and 2 contain the big-endian payload length.
    int payloadLength =
        (static_cast<int>(packet[1]) << 8) |
         static_cast<int>(packet[2]);

    if (payloadLength + 4 > maxLength)
    {
        std::cerr
            << "    AMBE packet too large"
            << std::endl;

        FT_Purge(handle, FT_PURGE_RX);

        return 0;
    }

    if (!ReadExact(
            handle,
            &packet[4],
            static_cast<DWORD>(payloadLength)))
    {
        return 0;
    }

    return payloadLength + 4;
}

static bool ResetAmbe(
    FT_HANDLE handle,
    const std::string& description)
{
    std::cout
        << "    Sending AMBE soft reset..."
        << std::endl;

    unsigned char zeros[10] = {};

    /*
     * This mirrors the existing AMBEd reset sequence.
     *
     * Send 350 zero bytes first in case the AMBE chip is
     * sitting in the middle of an incomplete UART packet.
     */
    for (int i = 0; i < 35; ++i)
    {
        if (!WriteExact(
                handle,
                zeros,
                sizeof(zeros)))
        {
            return false;
        }
    }

    unsigned char resetPacket[7] =
    {
        PKT_HEADER,
        0x00,
        0x03,
        PKT_CONTROL,
        PKT_RESET,
        PKT_PARITYBYTE,
        static_cast<unsigned char>(
            0x03 ^
            PKT_RESET ^
            PKT_PARITYBYTE)
    };

    if (!WriteExact(
            handle,
            resetPacket,
            sizeof(resetPacket)))
    {
        return false;
    }

    unsigned char reply[128] = {};

    int length =
        ReadPacket(
            handle,
            reply,
            sizeof(reply));

    /*
     * Existing AMBEd expects different reset packet lengths
     * for these device families:
     *
     * USB-3000 / AMBE3000 : 5 bytes total
     * USB-3003 / AMBE3003 : 7 bytes total
     *
     * DVstick-33 uses the AMBE3003 path.
     */
    int expectedLength = 0;

    if (description == "DVstick-33")
    {
        expectedLength = 7;
    }
    else if (description == "USB-3000")
    {
        expectedLength = 5;
    }

    std::cout
        << "    Reset reply length: "
        << length
        << " bytes"
        << std::endl;

    if (length > 4)
    {
        std::cout
            << "    Reset reply command: 0x"
            << std::hex
            << std::uppercase
            << static_cast<int>(reply[4])
            << std::dec
            << std::endl;
    }

    if (expectedLength == 0)
    {
        std::cerr
            << "    Unknown AMBE reset response format for "
            << description
            << std::endl;

        return false;
    }

    if (length == expectedLength &&
        reply[4] == PKT_READY)
    {
        std::cout
            << "    AMBE READY response received"
            << std::endl;

        return true;
    }

    std::cerr
        << "    AMBE reset failed"
        << std::endl;

    return false;
}

static bool ReadAmbeVersion(FT_HANDLE handle)
{
    unsigned char request[8] =
    {
        PKT_HEADER,
        0x00,
        0x04,
        PKT_CONTROL,
        PKT_PRODID,
        PKT_VERSTRING,
        PKT_PARITYBYTE,
        static_cast<unsigned char>(
            0x04 ^
            PKT_CONTROL ^
            PKT_PRODID ^
            PKT_VERSTRING ^
            PKT_PARITYBYTE)
    };

    std::cout
        << "    Requesting AMBE product/version..."
        << std::endl;

    if (!WriteExact(
            handle,
            request,
            sizeof(request)))
    {
        return false;
    }

    unsigned char reply[128] = {};

    int length =
        ReadPacket(
            handle,
            reply,
            sizeof(reply));

    if (length <= 4)
    {
        std::cerr
            << "    No AMBE version response"
            << std::endl;

        return false;
    }

    std::cout
        << "    AMBE response: ";

    int payloadEnd = length;

    /*
     * The reply contains the product string followed by the
     * firmware/version string.
     */
    int i = 5;

    while (i < payloadEnd &&
           reply[i] != 0x00)
    {
        std::cout
            << static_cast<char>(reply[i]);

        ++i;
    }

    std::cout << " ";

    /*
     * Skip the product-string terminator and the following
     * field identifier before printing the version string.
     */
    i += 2;

    while (i < payloadEnd &&
           reply[i] != 0x00)
    {
        std::cout
            << static_cast<char>(reply[i]);

        ++i;
    }

    std::cout << std::endl;

    return true;
}

static bool TestDevice(
    const FT_DEVICE_LIST_INFO_NODE& dev)
{
    const std::string description(
        dev.Description);

    const std::string serial(
        dev.SerialNumber);

    DWORD baudRate = 0;

    if (description == "DVstick-33")
    {
        baudRate = 921600;
    }
    else if (description == "USB-3000")
    {
        baudRate = 460800;
    }
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
        << " ("
        << serial
        << ")"
        << std::endl;

    FT_HANDLE handle = nullptr;

    if (!Check(
            "FT_OpenEx",
            FT_OpenEx(
                (PVOID)dev.SerialNumber,
                FT_OPEN_BY_SERIAL_NUMBER,
                &handle)))
    {
        return false;
    }

    bool ok = true;

    Sleep(50);

    ok &= Check(
        "FT_Purge",
        FT_Purge(
            handle,
            FT_PURGE_RX | FT_PURGE_TX));

    Sleep(50);

    ok &= Check(
        "FT_SetDataCharacteristics",
        FT_SetDataCharacteristics(
            handle,
            FT_BITS_8,
            FT_STOP_BITS_1,
            FT_PARITY_NONE));

    ok &= Check(
        "FT_SetFlowControl",
        FT_SetFlowControl(
            handle,
            FT_FLOW_RTS_CTS,
            0x11,
            0x13));

    ok &= Check(
        "FT_SetRts",
        FT_SetRts(handle));

    ok &= Check(
        "FT_ClrDtr",
        FT_ClrDtr(handle));

    std::cout
        << "    Baud rate: "
        << baudRate
        << std::endl;

    ok &= Check(
        "FT_SetBaudRate",
        FT_SetBaudRate(
            handle,
            baudRate));

    ok &= Check(
        "FT_SetLatencyTimer",
        FT_SetLatencyTimer(
            handle,
            4));

    ok &= Check(
        "FT_SetUSBParameters",
        FT_SetUSBParameters(
            handle,
            1024,
            0));

    ok &= Check(
        "FT_SetTimeouts",
        FT_SetTimeouts(
            handle,
            200,
            200));

    if (ok)
    {
        if (!ResetAmbe(
                handle,
                description))
        {
            ok = false;
        }
    }

    if (ok)
    {
        if (!ReadAmbeVersion(handle))
        {
            ok = false;
        }
    }

    if (!Check(
            "FT_Close",
            FT_Close(handle)))
    {
        ok = false;
    }

    std::cout
        << (ok
            ? "  AMBE device test PASSED"
            : "  AMBE device test FAILED")
        << std::endl;

    return ok;
}

int main()
{
    DWORD deviceCount = 0;

    if (!Check(
            "FT_CreateDeviceInfoList",
            FT_CreateDeviceInfoList(
                &deviceCount)))
    {
        return 1;
    }

    std::cout
        << std::endl
        << "Detected "
        << deviceCount
        << " FTDI device(s)"
        << std::endl;

    if (deviceCount == 0)
        return 0;

    std::vector<FT_DEVICE_LIST_INFO_NODE>
        devices(deviceCount);

    if (!Check(
            "FT_GetDeviceInfoList",
            FT_GetDeviceInfoList(
                devices.data(),
                &deviceCount)))
    {
        return 1;
    }

    bool allOK = true;

    for (DWORD i = 0;
         i < deviceCount;
         ++i)
    {
        const auto& dev =
            devices[i];

        /*
         * D2XX stores the FTDI ID as:
         *
         * high word = VID
         * low word  = PID
         */
        WORD vid =
            HIWORD(dev.ID);

        WORD pid =
            LOWORD(dev.ID);

        std::cout << std::endl;

        std::cout
            << "["
            << i
            << "]"
            << std::endl;

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
            << std::setw(4)
            << vid
            << ":"
            << std::setw(4)
            << pid
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

        if (!TestDevice(dev))
        {
            allOK = false;
        }
    }

    std::cout << std::endl;

    if (allOK)
    {
        std::cout
            << "All supported AMBE devices responded successfully."
            << std::endl;

        return 0;
    }

    std::cerr
        << "One or more AMBE device tests failed."
        << std::endl;

    return 1;
}