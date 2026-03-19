



#pragma once
#ifndef INCLUDED_AI_NULLLOGGER_H
#define INCLUDED_AI_NULLLOGGER_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include "Logger.hpp"

namespace Assimp {



class ASSIMP_API NullLogger
    : public Logger {

public:

    
    void OnDebug(const char* message) {
        (void)message; 
    }

    
	void OnVerboseDebug(const char *message) {
		(void)message; 
	}

    
    void OnInfo(const char* message) {
        (void)message; 
    }

    
    void OnWarn(const char* message) {
        (void)message; 
    }

    
    void OnError(const char* message) {
        (void)message; 
    }

    
    bool attachStream(LogStream *pStream, unsigned int severity) {
        (void)pStream; (void)severity; 
        return false;
    }

    
    bool detachStream(LogStream *pStream, unsigned int severity) {
        (void)pStream; (void)severity; 
        return false;
    }

private:
};
}
#endif 
