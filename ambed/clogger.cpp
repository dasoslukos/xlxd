//
//  clogger.cpp
//  ambed
//
//  Cross-platform console/file logging helper.
//

#include "clogger.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>

namespace
{
    constexpr std::uintmax_t LOG_ROTATE_SIZE = 5ULL * 1024ULL * 1024ULL;

    std::string MakeTimestamp(void)
    {
        using namespace std::chrono;

        const auto now = system_clock::now();
        const auto nowTime = system_clock::to_time_t(now);
        const auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::tm localTime {};

#ifdef _WIN32
        ::localtime_s(&localTime, &nowTime);
#else
        ::localtime_r(&nowTime, &localTime);
#endif

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
               << '.'
               << std::setfill('0') << std::setw(3) << millis.count();

        return stream.str();
    }
}

class CLogBuffer : public std::streambuf
{
public:
    CLogBuffer(std::streambuf *consoleBuffer,
               std::ofstream *file,
               std::mutex *mutex,
               const char *level)
        : m_ConsoleBuffer(consoleBuffer),
          m_File(file),
          m_Mutex(mutex),
          m_Level(level)
    {
    }

    void FlushPending(void)
    {
        std::lock_guard<std::mutex> lock(*m_Mutex);

        if (!m_Pending.empty())
        {
            WriteFileLine(m_Pending);
            m_Pending.clear();
        }

        if (m_File != nullptr)
        {
            m_File->flush();
        }

        if (m_ConsoleBuffer != nullptr)
        {
            m_ConsoleBuffer->pubsync();
        }
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
        {
            return traits_type::not_eof(ch);
        }

        const char c = traits_type::to_char_type(ch);

        std::lock_guard<std::mutex> lock(*m_Mutex);

        if (m_ConsoleBuffer != nullptr)
        {
            m_ConsoleBuffer->sputc(c);
        }

        ProcessCharacter(c);

        return ch;
    }

    std::streamsize xsputn(const char *data, std::streamsize count) override
    {
        std::lock_guard<std::mutex> lock(*m_Mutex);

        if (m_ConsoleBuffer != nullptr)
        {
            m_ConsoleBuffer->sputn(data, count);
        }

        for (std::streamsize i = 0; i < count; ++i)
        {
            ProcessCharacter(data[i]);
        }

        return count;
    }

    int sync(void) override
    {
        std::lock_guard<std::mutex> lock(*m_Mutex);

        if (m_File != nullptr)
        {
            m_File->flush();
        }

        if (m_ConsoleBuffer != nullptr)
        {
            m_ConsoleBuffer->pubsync();
        }

        return 0;
    }

private:
    void ProcessCharacter(char c)
    {
        if (c == '\r')
        {
            return;
        }

        if (c == '\n')
        {
            WriteFileLine(m_Pending);
            m_Pending.clear();
            return;
        }

        m_Pending.push_back(c);
    }

    void WriteFileLine(const std::string &line)
    {
        if ((m_File == nullptr) || !m_File->is_open())
        {
            return;
        }

        (*m_File) << MakeTimestamp()
                  << " [" << m_Level << "] "
                  << line
                  << '\n';

        m_File->flush();
    }

private:
    std::streambuf *m_ConsoleBuffer;
    std::ofstream  *m_File;
    std::mutex     *m_Mutex;
    std::string     m_Level;
    std::string     m_Pending;
};

CLogger::CLogger()
{
    m_bStarted = false;
    m_bMirrorConsole = true;
    m_OriginalCout = nullptr;
    m_OriginalCerr = nullptr;
    m_OriginalClog = nullptr;
}

CLogger::~CLogger()
{
    Stop();
}

bool CLogger::RotateAtStartup(const std::string &logPath)
{
    namespace fs = std::filesystem;

    std::error_code error;
    const fs::path path = fs::u8path(logPath);

    if (!fs::exists(path, error) || error)
    {
        return true;
    }

    const std::uintmax_t size = fs::file_size(path, error);
    if (error || (size < LOG_ROTATE_SIZE))
    {
        return true;
    }

    fs::path oldPath = path;
    oldPath += ".1";

    fs::remove(oldPath, error);
    error.clear();

    fs::rename(path, oldPath, error);

    return !error;
}

bool CLogger::Start(const std::string &logPath, bool mirrorConsole)
{
    if (m_bStarted)
    {
        return true;
    }

    namespace fs = std::filesystem;

    m_LogPath = logPath;
    m_bMirrorConsole = mirrorConsole;

    std::error_code error;
    const fs::path path = fs::u8path(logPath);
    const fs::path parent = path.parent_path();

    if (!parent.empty())
    {
        fs::create_directories(parent, error);
        if (error)
        {
            return false;
        }
    }

    if (!RotateAtStartup(logPath))
    {
        return false;
    }

    m_File.open(path, std::ios::out | std::ios::app);
    if (!m_File.is_open())
    {
        return false;
    }

    m_OriginalCout = std::cout.rdbuf();
    m_OriginalCerr = std::cerr.rdbuf();
    m_OriginalClog = std::clog.rdbuf();

    m_CoutBuffer = std::make_unique<CLogBuffer>(
        mirrorConsole ? m_OriginalCout : nullptr,
        &m_File,
        &m_Mutex,
        "INFO");

    m_CerrBuffer = std::make_unique<CLogBuffer>(
        mirrorConsole ? m_OriginalCerr : nullptr,
        &m_File,
        &m_Mutex,
        "ERROR");

    m_ClogBuffer = std::make_unique<CLogBuffer>(
        mirrorConsole ? m_OriginalClog : nullptr,
        &m_File,
        &m_Mutex,
        "INFO");

    std::cout.rdbuf(m_CoutBuffer.get());
    std::cerr.rdbuf(m_CerrBuffer.get());
    std::clog.rdbuf(m_ClogBuffer.get());

    m_bStarted = true;
    return true;
}

void CLogger::Stop(void)
{
    if (!m_bStarted)
    {
        return;
    }

    m_CoutBuffer->FlushPending();
    m_CerrBuffer->FlushPending();
    m_ClogBuffer->FlushPending();

    std::cout.rdbuf(m_OriginalCout);
    std::cerr.rdbuf(m_OriginalCerr);
    std::clog.rdbuf(m_OriginalClog);

    m_CoutBuffer.reset();
    m_CerrBuffer.reset();
    m_ClogBuffer.reset();

    m_File.flush();
    m_File.close();

    m_OriginalCout = nullptr;
    m_OriginalCerr = nullptr;
    m_OriginalClog = nullptr;
    m_bStarted = false;
}
