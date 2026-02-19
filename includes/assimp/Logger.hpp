


#pragma once
#ifndef INCLUDED_AI_LOGGER_H
#define INCLUDED_AI_LOGGER_H

#include <assimp/types.h>
#include <assimp/TinyFormatter.h>

namespace Assimp {

class LogStream;


#define MAX_LOG_MESSAGE_LENGTH 1024u



class ASSIMP_API Logger
#ifndef SWIG
    : public Intern::AllocateFromAssimpHeap
#endif
{
public:

    
    
    enum LogSeverity {
        NORMAL,     
        DEBUGGING,  
        VERBOSE     
    };

    
    
    enum ErrorSeverity {
        Debugging   = 1,    
        Info        = 2,    
        Warn        = 4,    
        Err         = 8     
    };

    
    virtual ~Logger();

    
    
    void debug(const char* message);

    template<typename... T>
    void debug(T&&... args) {
        debug(formatMessage(std::forward<T>(args)...).c_str());
    }

    
    
    void verboseDebug(const char* message);

    template<typename... T>
    void verboseDebug(T&&... args) {
        verboseDebug(formatMessage(std::forward<T>(args)...).c_str());
    }

    
    
    void info(const char* message);

    template<typename... T>
    void info(T&&... args) {
        info(formatMessage(std::forward<T>(args)...).c_str());
    }

    
    
    void warn(const char* message);

    template<typename... T>
    void warn(T&&... args) {
        warn(formatMessage(std::forward<T>(args)...).c_str());
    }

    
    
    void error(const char* message);

    template<typename... T>
    void error(T&&... args) {
        error(formatMessage(std::forward<T>(args)...).c_str());
    }

    
    
    void setLogSeverity(LogSeverity log_severity);

    
    
    LogSeverity getLogSeverity() const;

    
    
    virtual bool attachStream(LogStream *pStream,
        unsigned int severity = Debugging | Err | Warn | Info) = 0;

    
    
    virtual bool detachStream(LogStream *pStream,
        unsigned int severity = Debugging | Err | Warn | Info) = 0;

protected:
    
    Logger() AI_NO_EXCEPT;

    
    explicit Logger(LogSeverity severity);

    
    
    virtual void OnDebug(const char* message)= 0;

    
	
	virtual void OnVerboseDebug(const char *message) = 0;

    
    
    virtual void OnInfo(const char* message) = 0;

    
    
    virtual void OnWarn(const char* essage) = 0;

    
    
    virtual void OnError(const char* message) = 0;
protected:
    std::string formatMessage(Assimp::Formatter::format f) {
        return f;
    }

    template<typename... T, typename U>
    std::string formatMessage(Assimp::Formatter::format f, U&& u, T&&... args) {
        return formatMessage(std::move(f << std::forward<U>(u)), std::forward<T>(args)...);
    }

protected:
    LogSeverity m_Severity;
};


inline Logger::Logger() AI_NO_EXCEPT :
        m_Severity(NORMAL) {
    
}


inline Logger::~Logger() = default;


inline Logger::Logger(LogSeverity severity) :
        m_Severity(severity) {
    
}


inline void Logger::setLogSeverity(LogSeverity log_severity){
    m_Severity = log_severity;
}



inline Logger::LogSeverity Logger::getLogSeverity() const {
    return m_Severity;
}

} 


#define ASSIMP_LOG_WARN(...) \
	Assimp::DefaultLogger::get()->warn(__VA_ARGS__)

#define ASSIMP_LOG_ERROR(...) \
	Assimp::DefaultLogger::get()->error(__VA_ARGS__)

#define ASSIMP_LOG_DEBUG(...) \
	Assimp::DefaultLogger::get()->debug(__VA_ARGS__)

#define ASSIMP_LOG_VERBOSE_DEBUG(...) \
	Assimp::DefaultLogger::get()->verboseDebug(__VA_ARGS__)

#define ASSIMP_LOG_INFO(...) \
	Assimp::DefaultLogger::get()->info(__VA_ARGS__)

#endif 
