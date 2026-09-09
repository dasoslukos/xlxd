//
//  cwindowsservice.h
//  ambed
//
//  Native Windows console/service wrapper.
//

#ifndef cwindowsservice_h
#define cwindowsservice_h

#ifdef _WIN32

#include <string>

namespace WindowsService
{
    int RunConsole(const std::string &listenIp,
                   const std::string &logPath);

    int RunService(const std::string &listenIp,
                   const std::string &logPath);

    int Install(const std::string &listenIp,
                const std::string &logPath);

    int Uninstall(void);

    std::string DefaultServiceLogPath(void);
}

#endif /* _WIN32 */

#endif /* cwindowsservice_h */
