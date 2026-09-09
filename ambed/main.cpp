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

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#else
#include "syslog.h"
#include <sys/stat.h>
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
// global objects


////////////////////////////////////////////////////////////////////////////////////////
// function declaration


int main(int argc, const char * argv[])
{
#if defined(RUN_AS_DAEMON) && !defined(_WIN32)

    // redirect cout, cerr and clog to syslog
    syslog::redirect cout_redir(std::cout);
    syslog::redirect cerr_redir(std::cerr);
    syslog::redirect clog_redir(std::clog);

    //Fork the Parent Process
    pid_t pid, sid;
    pid = ::fork();
    //pid = ::vfork();
    if ( pid < 0 )
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

    //Create a new Signature Id for our child
    sid = ::setsid();
    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    // Change Directory
    // If we cant find the directory we exit with failure.
    if ( (::chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

#endif

    // check arguments
    if ( argc != 2 )
    {
        std::cout << "Usage: ambed ip" << std::endl;
        std::cout << "example: ambed 192.168.178.212" << std::endl;
        return 1;
    }

#ifdef _WIN32
    // Winsock must be initialized before any socket or name-resolution calls.
    WSADATA wsaData {};
    const int wsaResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (wsaResult != 0)
    {
        std::cerr << "WSAStartup failed with error " << wsaResult << std::endl;
        return EXIT_FAILURE;
    }

    // AMBEd's vocoder processing loop relies on millisecond-scale sleeps.
    // Request 1 ms timer resolution so a 2 ms sleep does not become ~15 ms.
    if (::timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        std::cerr << "timeBeginPeriod(1) failed" << std::endl;
        ::WSACleanup();
        return EXIT_FAILURE;
    }
#endif

    // initialize ambeserver
    g_AmbeServer.SetListenIp(CIp(argv[1]));

    // and let it run
    std::cout << "Starting AMBEd " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << std::endl << std::endl;
    if ( !g_AmbeServer.Start() )
    {
        std::cout << "Error starting AMBEd" << std::endl;
#ifdef _WIN32
        ::timeEndPeriod(1);
        ::WSACleanup();
#endif
        return EXIT_FAILURE;
    }
    std::cout << "AMBEd started and listening on " << g_AmbeServer.GetListenIp() << std::endl;

#if defined(RUN_AS_DAEMON) && !defined(_WIN32)
    // run forever
    while ( true )
    {
        // sleep 60 seconds
        CTimePoint::TaskSleepFor(60000);
    }
#else
    // wait any key
    for (;;)
    {
        // sleep 60 seconds
        CTimePoint::TaskSleepFor(60000);
        //std::cin.get();
    }
#endif

    // and wait for end
    g_AmbeServer.Stop();
    std::cout << "AMBEd stopped" << std::endl;

#ifdef _WIN32
    ::timeEndPeriod(1);
    ::WSACleanup();
#endif

    // done
    return EXIT_SUCCESS;
}
