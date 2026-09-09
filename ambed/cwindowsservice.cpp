//
//  cwindowsservice.cpp
//  ambed
//
//  Native Windows console/service wrapper.
//

#ifdef _WIN32

#include "main.h"
#include "cambedruntime.h"
#include "cwindowsservice.h"

#include <windows.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr wchar_t SERVICE_NAME[] = L"AMBEd";
    constexpr wchar_t SERVICE_DISPLAY_NAME[] = L"AMBEd Transcoder";

    HANDLE                g_ConsoleStopEvent = nullptr;
    HANDLE                g_ServiceStopEvent = nullptr;
    SERVICE_STATUS_HANDLE g_ServiceStatusHandle = nullptr;
    SERVICE_STATUS        g_ServiceStatus {};
    DWORD                 g_ServiceCheckPoint = 1;

    std::string           g_ServiceListenIp;
    std::string           g_ServiceLogPath;

    std::wstring Utf8ToWide(const std::string &value)
    {
        if (value.empty())
        {
            return std::wstring();
        }

        const int length = ::MultiByteToWideChar(
            CP_UTF8, 0, value.c_str(), -1, nullptr, 0);

        if (length <= 0)
        {
            return std::wstring();
        }

        std::wstring result(static_cast<std::size_t>(length), L'\0');

        ::MultiByteToWideChar(
            CP_UTF8, 0, value.c_str(), -1, result.data(), length);

        result.resize(static_cast<std::size_t>(length - 1));
        return result;
    }

    std::string WideToUtf8(const std::wstring &value)
    {
        if (value.empty())
        {
            return std::string();
        }

        const int length = ::WideCharToMultiByte(
            CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);

        if (length <= 0)
        {
            return std::string();
        }

        std::string result(static_cast<std::size_t>(length), '\0');

        ::WideCharToMultiByte(
            CP_UTF8, 0, value.c_str(), -1, result.data(), length, nullptr, nullptr);

        result.resize(static_cast<std::size_t>(length - 1));
        return result;
    }

    std::wstring QuoteArgument(const std::wstring &value)
    {
        std::wstring result = L"\"";

        for (const wchar_t c : value)
        {
            if (c == L'"')
            {
                result += L"\\\"";
            }
            else
            {
                result += c;
            }
        }

        result += L"\"";
        return result;
    }

    std::wstring GetExecutablePath(void)
    {
        std::vector<wchar_t> buffer(32768);

        const DWORD length = ::GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size()));

        if ((length == 0) || (length >= buffer.size()))
        {
            return std::wstring();
        }

        return std::wstring(buffer.data(), length);
    }

    void PrintWin32Error(const char *operation, DWORD error)
    {
        LPWSTR messageBuffer = nullptr;

        const DWORD chars = ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&messageBuffer),
            0,
            nullptr);

        std::cerr << operation << " failed with error " << error;

        if ((chars != 0) && (messageBuffer != nullptr))
        {
            std::wstring message(messageBuffer, chars);

            while (!message.empty() &&
                   ((message.back() == L'\r') || (message.back() == L'\n')))
            {
                message.pop_back();
            }

            std::cerr << ": " << WideToUtf8(message);
        }

        std::cerr << std::endl;

        if (messageBuffer != nullptr)
        {
            ::LocalFree(messageBuffer);
        }
    }

    BOOL WINAPI ConsoleControlHandler(DWORD controlType)
    {
        switch (controlType)
        {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_CLOSE_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                if (g_ConsoleStopEvent != nullptr)
                {
                    ::SetEvent(g_ConsoleStopEvent);
                }
                return TRUE;

            default:
                return FALSE;
        }
    }

    void ReportServiceStatus(DWORD state,
                             DWORD win32ExitCode = NO_ERROR,
                             DWORD waitHint = 0,
                             DWORD serviceSpecificExitCode = 0)
    {
        if (g_ServiceStatusHandle == nullptr)
        {
            return;
        }

        g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_ServiceStatus.dwCurrentState = state;
        g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
        g_ServiceStatus.dwServiceSpecificExitCode = serviceSpecificExitCode;
        g_ServiceStatus.dwWaitHint = waitHint;

        if (state == SERVICE_RUNNING)
        {
            g_ServiceStatus.dwControlsAccepted =
                SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
            g_ServiceStatus.dwCheckPoint = 0;
            g_ServiceCheckPoint = 1;
        }
        else if ((state == SERVICE_START_PENDING) ||
                 (state == SERVICE_STOP_PENDING))
        {
            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCheckPoint = g_ServiceCheckPoint++;
        }
        else
        {
            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCheckPoint = 0;
        }

        ::SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
    }

    DWORD WINAPI ServiceControlHandler(DWORD control,
                                       DWORD,
                                       LPVOID,
                                       LPVOID)
    {
        switch (control)
        {
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                if ((g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (g_ServiceStatus.dwCurrentState == SERVICE_START_PENDING))
                {
                    ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 15000);

                    if (g_ServiceStopEvent != nullptr)
                    {
                        ::SetEvent(g_ServiceStopEvent);
                    }
                }
                return NO_ERROR;

            case SERVICE_CONTROL_INTERROGATE:
                return NO_ERROR;

            default:
                return ERROR_CALL_NOT_IMPLEMENTED;
        }
    }

    void WINAPI ServiceMain(DWORD, LPWSTR *)
    {
        g_ServiceStatusHandle = ::RegisterServiceCtrlHandlerExW(
            SERVICE_NAME,
            ServiceControlHandler,
            nullptr);

        if (g_ServiceStatusHandle == nullptr)
        {
            return;
        }

        ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 15000);

        g_ServiceStopEvent = ::CreateEventW(
            nullptr,
            TRUE,
            FALSE,
            nullptr);

        if (g_ServiceStopEvent == nullptr)
        {
            ReportServiceStatus(
                SERVICE_STOPPED,
                ::GetLastError(),
                0);

            return;
        }

        CAmbedRuntime runtime;

        if (!runtime.Start(
                g_ServiceListenIp,
                g_ServiceLogPath,
                false))
        {
            ::CloseHandle(g_ServiceStopEvent);
            g_ServiceStopEvent = nullptr;

            ReportServiceStatus(
                SERVICE_STOPPED,
                ERROR_SERVICE_SPECIFIC_ERROR,
                0,
                1);

            return;
        }

        ReportServiceStatus(SERVICE_RUNNING);

        ::WaitForSingleObject(g_ServiceStopEvent, INFINITE);

        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 15000);

        runtime.Stop();

        ::CloseHandle(g_ServiceStopEvent);
        g_ServiceStopEvent = nullptr;

        ReportServiceStatus(SERVICE_STOPPED);
    }
}

namespace WindowsService
{
    std::string DefaultServiceLogPath(void)
    {
        wchar_t programData[32768] {};
        const DWORD length = ::GetEnvironmentVariableW(
            L"ProgramData",
            programData,
            static_cast<DWORD>(sizeof(programData) / sizeof(programData[0])));

        std::wstring path;

        if ((length > 0) &&
            (length < (sizeof(programData) / sizeof(programData[0]))))
        {
            path.assign(programData, length);
        }
        else
        {
            path = L"C:\\ProgramData";
        }

        if (!path.empty() && (path.back() != L'\\'))
        {
            path += L'\\';
        }

        path += L"AMBEd\\ambed.log";

        return WideToUtf8(path);
    }

    int RunConsole(const std::string &listenIp,
                   const std::string &logPath)
    {
        g_ConsoleStopEvent = ::CreateEventW(
            nullptr,
            TRUE,
            FALSE,
            nullptr);

        if (g_ConsoleStopEvent == nullptr)
        {
            PrintWin32Error("CreateEvent", ::GetLastError());
            return EXIT_FAILURE;
        }

        if (!::SetConsoleCtrlHandler(ConsoleControlHandler, TRUE))
        {
            PrintWin32Error("SetConsoleCtrlHandler", ::GetLastError());
            ::CloseHandle(g_ConsoleStopEvent);
            g_ConsoleStopEvent = nullptr;
            return EXIT_FAILURE;
        }

        CAmbedRuntime runtime;

        if (!runtime.Start(listenIp, logPath, true))
        {
            ::SetConsoleCtrlHandler(ConsoleControlHandler, FALSE);
            ::CloseHandle(g_ConsoleStopEvent);
            g_ConsoleStopEvent = nullptr;
            return EXIT_FAILURE;
        }

        std::cout << "Press Ctrl+C to stop AMBEd gracefully" << std::endl;

        ::WaitForSingleObject(g_ConsoleStopEvent, INFINITE);

        std::cout << "Shutdown requested" << std::endl;

        runtime.Stop();

        ::SetConsoleCtrlHandler(ConsoleControlHandler, FALSE);
        ::CloseHandle(g_ConsoleStopEvent);
        g_ConsoleStopEvent = nullptr;

        return EXIT_SUCCESS;
    }

    int RunService(const std::string &listenIp,
                   const std::string &logPath)
    {
        g_ServiceListenIp = listenIp;
        g_ServiceLogPath = logPath;

        SERVICE_TABLE_ENTRYW serviceTable[] =
        {
            {
                const_cast<LPWSTR>(SERVICE_NAME),
                ServiceMain
            },
            {
                nullptr,
                nullptr
            }
        };

        if (!::StartServiceCtrlDispatcherW(serviceTable))
        {
            const DWORD error = ::GetLastError();
            PrintWin32Error("StartServiceCtrlDispatcher", error);

            if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
            {
                std::cerr
                    << "The --service mode must be started by the Windows "
                    << "Service Control Manager."
                    << std::endl;
            }

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    int Install(const std::string &listenIp,
                const std::string &logPath)
    {
        const std::wstring executablePath = GetExecutablePath();

        if (executablePath.empty())
        {
            PrintWin32Error("GetModuleFileName", ::GetLastError());
            return EXIT_FAILURE;
        }

        const std::wstring listenIpWide = Utf8ToWide(listenIp);
        const std::wstring logPathWide = Utf8ToWide(logPath);

        if (listenIpWide.empty() || logPathWide.empty())
        {
            std::cerr << "Unable to convert service arguments to Unicode" << std::endl;
            return EXIT_FAILURE;
        }

        const std::wstring imagePath =
            QuoteArgument(executablePath) +
            L" --service " +
            QuoteArgument(listenIpWide) +
            L" --log " +
            QuoteArgument(logPathWide);

        SC_HANDLE manager = ::OpenSCManagerW(
            nullptr,
            nullptr,
            SC_MANAGER_CREATE_SERVICE);

        if (manager == nullptr)
        {
            PrintWin32Error("OpenSCManager", ::GetLastError());
            return EXIT_FAILURE;
        }

        SC_HANDLE service = ::CreateServiceW(
            manager,
            SERVICE_NAME,
            SERVICE_DISPLAY_NAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            imagePath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        if (service == nullptr)
        {
            const DWORD error = ::GetLastError();
            PrintWin32Error("CreateService", error);

            if (error == ERROR_SERVICE_EXISTS)
            {
                std::cerr
                    << "AMBEd is already installed as a Windows service."
                    << std::endl;
            }

            ::CloseServiceHandle(manager);
            return EXIT_FAILURE;
        }

        SERVICE_DESCRIPTIONW description {};
        description.lpDescription =
            const_cast<LPWSTR>(
                L"Native AMBEd hardware transcoder service for XLX.");

        ::ChangeServiceConfig2W(
            service,
            SERVICE_CONFIG_DESCRIPTION,
            &description);

        // Give USB/FTDI devices time to settle during boot.
        SERVICE_DELAYED_AUTO_START_INFO delayedStart {};
        delayedStart.fDelayedAutostart = TRUE;

        ::ChangeServiceConfig2W(
            service,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            &delayedStart);

        // Restart automatically after the first two unexpected failures.
        SC_ACTION actions[3] =
        {
            { SC_ACTION_RESTART, 5000 },
            { SC_ACTION_RESTART, 5000 },
            { SC_ACTION_NONE, 0 }
        };

        SERVICE_FAILURE_ACTIONSW failureActions {};
        failureActions.dwResetPeriod = 86400;
        failureActions.cActions = 3;
        failureActions.lpsaActions = actions;

        ::ChangeServiceConfig2W(
            service,
            SERVICE_CONFIG_FAILURE_ACTIONS,
            &failureActions);

        ::CloseServiceHandle(service);
        ::CloseServiceHandle(manager);

        std::cout << "AMBEd Windows service installed successfully" << std::endl;
        std::cout << "Executable: " << WideToUtf8(executablePath) << std::endl;
        std::cout << "Bind IP:    " << listenIp << std::endl;
        std::cout << "Log file:   " << logPath << std::endl;
        std::cout << "Startup:    Automatic (Delayed Start)" << std::endl;
        std::cout << "Start it with: sc.exe start AMBEd" << std::endl;

        return EXIT_SUCCESS;
    }

    int Uninstall(void)
    {
        SC_HANDLE manager = ::OpenSCManagerW(
            nullptr,
            nullptr,
            SC_MANAGER_CONNECT);

        if (manager == nullptr)
        {
            PrintWin32Error("OpenSCManager", ::GetLastError());
            return EXIT_FAILURE;
        }

        SC_HANDLE service = ::OpenServiceW(
            manager,
            SERVICE_NAME,
            SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

        if (service == nullptr)
        {
            PrintWin32Error("OpenService", ::GetLastError());
            ::CloseServiceHandle(manager);
            return EXIT_FAILURE;
        }

        SERVICE_STATUS_PROCESS status {};
        DWORD bytesNeeded = 0;

        if (::QueryServiceStatusEx(
                service,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&status),
                sizeof(status),
                &bytesNeeded))
        {
            if (status.dwCurrentState != SERVICE_STOPPED)
            {
                SERVICE_STATUS temporaryStatus {};
                ::ControlService(service, SERVICE_CONTROL_STOP, &temporaryStatus);

                const ULONGLONG deadline = ::GetTickCount64() + 15000;

                do
                {
                    ::Sleep(250);

                    if (!::QueryServiceStatusEx(
                            service,
                            SC_STATUS_PROCESS_INFO,
                            reinterpret_cast<LPBYTE>(&status),
                            sizeof(status),
                            &bytesNeeded))
                    {
                        break;
                    }

                    if (status.dwCurrentState == SERVICE_STOPPED)
                    {
                        break;
                    }
                }
                while (::GetTickCount64() < deadline);
            }
        }

        if (!::DeleteService(service))
        {
            PrintWin32Error("DeleteService", ::GetLastError());
            ::CloseServiceHandle(service);
            ::CloseServiceHandle(manager);
            return EXIT_FAILURE;
        }

        ::CloseServiceHandle(service);
        ::CloseServiceHandle(manager);

        std::cout << "AMBEd Windows service removed successfully" << std::endl;

        return EXIT_SUCCESS;
    }
}

#endif /* _WIN32 */
