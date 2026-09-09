//
//  cambedruntime.cpp
//  ambed
//
//  Shared runtime lifecycle used by the Windows console and service modes.
//

#include "main.h"
#include "cambedruntime.h"
#include "cambeserver.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

CAmbedRuntime::CAmbedRuntime()
{
    m_bRunning = false;
    m_bWinsockStarted = false;
    m_bTimerResolutionSet = false;
}

CAmbedRuntime::~CAmbedRuntime()
{
    Stop();
}

bool CAmbedRuntime::Start(const std::string &listenIp,
                          const std::string &logPath,
                          bool mirrorConsole)
{
    if (m_bRunning)
    {
        return true;
    }

    if (!m_Logger.Start(logPath, mirrorConsole))
    {
        std::cerr << "Unable to open log file: " << logPath << std::endl;
        return false;
    }

    std::cout << "Log file: " << logPath << std::endl;

#ifdef _WIN32
    WSADATA wsaData {};
    const int wsaResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (wsaResult != 0)
    {
        std::cerr << "WSAStartup failed with error " << wsaResult << std::endl;
        m_Logger.Stop();
        return false;
    }

    m_bWinsockStarted = true;

    // AMBEd's vocoder processing loop relies on millisecond-scale sleeps.
    // Request 1 ms timer resolution so a 2 ms sleep does not become ~15 ms.
    if (::timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        std::cerr << "timeBeginPeriod(1) failed" << std::endl;
        ::WSACleanup();
        m_bWinsockStarted = false;
        m_Logger.Stop();
        return false;
    }

    m_bTimerResolutionSet = true;
#endif

    g_AmbeServer.SetListenIp(CIp(listenIp.c_str()));

    std::cout << "Starting AMBEd "
              << VERSION_MAJOR << "."
              << VERSION_MINOR << "."
              << VERSION_REVISION
              << std::endl
              << std::endl;

    if (!g_AmbeServer.Start())
    {
        std::cerr << "Error starting AMBEd" << std::endl;

#ifdef _WIN32
        if (m_bTimerResolutionSet)
        {
            ::timeEndPeriod(1);
            m_bTimerResolutionSet = false;
        }

        if (m_bWinsockStarted)
        {
            ::WSACleanup();
            m_bWinsockStarted = false;
        }
#endif

        m_Logger.Stop();
        return false;
    }

    m_bRunning = true;

    std::cout << "AMBEd started and listening on "
              << g_AmbeServer.GetListenIp()
              << std::endl;

    return true;
}

void CAmbedRuntime::Stop(void)
{
    if (m_bRunning)
    {
        std::cout << "Stopping AMBEd" << std::endl;
        g_AmbeServer.Stop();
        m_bRunning = false;
        std::cout << "AMBEd stopped" << std::endl;
    }

#ifdef _WIN32
    if (m_bTimerResolutionSet)
    {
        ::timeEndPeriod(1);
        m_bTimerResolutionSet = false;
    }

    if (m_bWinsockStarted)
    {
        ::WSACleanup();
        m_bWinsockStarted = false;
    }
#endif

    m_Logger.Stop();
}
