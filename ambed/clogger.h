//
//  clogger.h
//  ambed
//
//  Cross-platform console/file logging helper.
//

#ifndef clogger_h
#define clogger_h

#include <fstream>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>

class CLogBuffer;

class CLogger
{
public:
    CLogger();
    ~CLogger();

    bool Start(const std::string &logPath, bool mirrorConsole);
    void Stop();

    bool IsStarted(void) const { return m_bStarted; }
    const std::string &GetPath(void) const { return m_LogPath; }

private:
    CLogger(const CLogger &) = delete;
    CLogger &operator=(const CLogger &) = delete;

    bool RotateAtStartup(const std::string &logPath);

private:
    bool                        m_bStarted;
    bool                        m_bMirrorConsole;
    std::string                 m_LogPath;
    std::ofstream               m_File;
    std::mutex                  m_Mutex;

    std::streambuf             *m_OriginalCout;
    std::streambuf             *m_OriginalCerr;
    std::streambuf             *m_OriginalClog;

    std::unique_ptr<CLogBuffer> m_CoutBuffer;
    std::unique_ptr<CLogBuffer> m_CerrBuffer;
    std::unique_ptr<CLogBuffer> m_ClogBuffer;
};

#endif /* clogger_h */
