



#pragma once
#ifndef INCLUDED_AI_DEFAULTLOGGER
#define INCLUDED_AI_DEFAULTLOGGER

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include "LogStream.hpp"
#include "Logger.hpp"
#include "NullLogger.hpp"
#include <vector>

#ifndef ASSIMP_BUILD_SINGLETHREADED
#include <mutex>
#include <thread>
#endif

namespace Assimp {

class IOStream;
struct LogStreamInfo;


#define ASSIMP_DEFAULT_LOG_NAME "AssimpLog.txt"



class ASSIMP_API DefaultLogger : public Logger {
public:
    
    
    static Logger *create(const char *name = ASSIMP_DEFAULT_LOG_NAME,
            LogSeverity severity = NORMAL,
            unsigned int defStreams = aiDefaultLogStream_DEBUGGER | aiDefaultLogStream_FILE,
            IOSystem *io = nullptr);

    
    
    static void set(Logger *logger);

    
    
    static Logger *get();

    
    
    static bool isNullLogger();

    
    
    static void kill();

    
    
    bool attachStream(LogStream *pStream, unsigned int severity) override;

    
    
    bool detachStream(LogStream *pStream, unsigned int severity) override;

private:
    
    
    explicit DefaultLogger(LogSeverity severity);

    
    
    ~DefaultLogger() override;

    
    void OnDebug(const char *message) override;

    
    void OnVerboseDebug(const char *message) override;

    
    void OnInfo(const char *message) override;

    
    void OnWarn(const char *message) override;

    
    void OnError(const char *message) override;

    
    
    void WriteToStreams(const char *message, ErrorSeverity ErrorSev);

    
    
    unsigned int GetThreadID();

private:
    
    using StreamArray = std::vector<LogStreamInfo *>;
    using StreamIt = std::vector<LogStreamInfo *>::iterator;
    using ConstStreamIt = std::vector<LogStreamInfo *>::const_iterator;

    
    static Logger *m_pLogger;
    static NullLogger s_pNullLogger;

    
    StreamArray m_StreamArray;

#ifndef ASSIMP_BUILD_SINGLETHREADED
    std::mutex m_arrayMutex;
#endif

    bool noRepeatMsg;
    char lastMsg[MAX_LOG_MESSAGE_LENGTH * 2];
    size_t lastLen;
};



} 

#endif 
