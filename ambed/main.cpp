//
//  main.cpp
//  ambed
//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/04/2017.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of ambed.
//
//    xlxd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    xlxd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include "ctimepoint.h"
#include "cambeserver.h"

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32

#include "cwindowsservice.h"

#else

#include "syslog.h"

#include <csignal>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
    volatile std::sig_atomic_t g_StopRequested = 0;

    void HandleSignal(int)
    {
        g_StopRequested = 1;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////
// helpers

#ifdef _WIN32

namespace
{
    void PrintUsage(void)
    {
        std::cout
            << "AMBEd " << VERSION_MAJOR << "."
            << VERSION_MINOR << "."
            << VERSION_REVISION << std::endl
            << std::endl
            << "Console mode:" << std::endl
            << "  ambed <ip> [--log <path>]" << std::endl
            << std::endl
            << "Windows service:" << std::endl
            << "  ambed --install <ip> [--log <path>]" << std::endl
            << "  ambed --uninstall" << std::endl
            << std::endl
            << "Internal SCM mode:" << std::endl
            << "  ambed --service <ip> [--log <path>]" << std::endl
            << std::endl
            << "Examples:" << std::endl
            << "  ambed 192.168.178.212" << std::endl
            << "  ambed --install 192.168.178.212" << std::endl;
    }

    bool ParseLogOption(int argc,
                        const char *argv[],
                        int startIndex,
                        const std::string &defaultLogPath,
                        std::string *logPath)
    {
        *logPath = defaultLogPath;

        int index = startIndex;

        while (index < argc)
        {
            if ((std::strcmp(argv[index], "--log") == 0) &&
                ((index + 1) < argc))
            {
                *logPath = argv[index + 1];
                index += 2;
            }
            else
            {
                return false;
            }
        }

        return true;
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////
// main

int main(int argc, const char *argv[])
{
#ifdef _WIN32

    if (argc < 2)
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    if ((std::strcmp(argv[1], "--help") == 0) ||
        (std::strcmp(argv[1], "-h") == 0) ||
        (std::strcmp(argv[1], "/?") == 0))
    {
        PrintUsage();
        return EXIT_SUCCESS;
    }

    if (std::strcmp(argv[1], "--version") == 0)
    {
        std::cout
            << "AMBEd "
            << VERSION_MAJOR << "."
            << VERSION_MINOR << "."
            << VERSION_REVISION
            << std::endl;

        return EXIT_SUCCESS;
    }

    if (std::strcmp(argv[1], "--uninstall") == 0)
    {
        if (argc != 2)
        {
            PrintUsage();
            return EXIT_FAILURE;
        }

        return WindowsService::Uninstall();
    }

    if (std::strcmp(argv[1], "--install") == 0)
    {
        if (argc < 3)
        {
            PrintUsage();
            return EXIT_FAILURE;
        }

        std::string logPath;

        if (!ParseLogOption(
                argc,
                argv,
                3,
                WindowsService::DefaultServiceLogPath(),
                &logPath))
        {
            PrintUsage();
            return EXIT_FAILURE;
        }

        return WindowsService::Install(argv[2], logPath);
    }

    if (std::strcmp(argv[1], "--service") == 0)
    {
        if (argc < 3)
        {
            return EXIT_FAILURE;
        }

        std::string logPath;

        if (!ParseLogOption(
                argc,
                argv,
                3,
                WindowsService::DefaultServiceLogPath(),
                &logPath))
        {
            return EXIT_FAILURE;
        }

        return WindowsService::RunService(argv[2], logPath);
    }

    std::string logPath;

    if (!ParseLogOption(
            argc,
            argv,
            2,
            "ambed.log",
            &logPath))
    {
        PrintUsage();
        return EXIT_FAILURE;
    }

    return WindowsService::RunConsole(argv[1], logPath);

#else

#if defined(RUN_AS_DAEMON)

    // redirect cout, cerr and clog to syslog
    syslog::redirect cout_redir(std::cout);
    syslog::redirect cerr_redir(std::cerr);
    syslog::redirect clog_redir(std::clog);

    // Fork the Parent Process
    pid_t pid, sid;
    pid = ::fork();

    if (pid < 0)
    {
        return EXIT_FAILURE;
    }

    // We got a good pid, Close the Parent Process
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // Change File Mask
    ::umask(0);

    // Create a new Session Id for our child
    sid = ::setsid();
    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    // Change Directory
    if ((::chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

#endif

    if (argc != 2)
    {
        std::cout << "Usage: ambed ip" << std::endl;
        std::cout << "example: ambed 192.168.178.212" << std::endl;
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    g_AmbeServer.SetListenIp(CIp(argv[1]));

    std::cout
        << "Starting AMBEd "
        << VERSION_MAJOR << "."
        << VERSION_MINOR << "."
        << VERSION_REVISION
        << std::endl
        << std::endl;

    if (!g_AmbeServer.Start())
    {
        std::cout << "Error starting AMBEd" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout
        << "AMBEd started and listening on "
        << g_AmbeServer.GetListenIp()
        << std::endl;

    while (!g_StopRequested)
    {
        CTimePoint::TaskSleepFor(200);
    }

    std::cout << "Shutdown requested" << std::endl;

    g_AmbeServer.Stop();
    std::cout << "AMBEd stopped" << std::endl;

    return EXIT_SUCCESS;

#endif
}
