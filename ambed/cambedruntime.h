//
//  cambedruntime.h
//  ambed
//
//  Shared runtime lifecycle used by the Windows console and service modes.
//

#ifndef cambedruntime_h
#define cambedruntime_h

#include "clogger.h"

#include <string>

class CAmbedRuntime
{
public:
    CAmbedRuntime();
    ~CAmbedRuntime();

    bool Start(const std::string &listenIp,
               const std::string &logPath,
               bool mirrorConsole);

    void Stop();

    bool IsRunning(void) const { return m_bRunning; }

private:
    CAmbedRuntime(const CAmbedRuntime &) = delete;
    CAmbedRuntime &operator=(const CAmbedRuntime &) = delete;

private:
    CLogger m_Logger;
    bool    m_bRunning;
    bool    m_bWinsockStarted;
    bool    m_bTimerResolutionSet;
};

#endif /* cambedruntime_h */
