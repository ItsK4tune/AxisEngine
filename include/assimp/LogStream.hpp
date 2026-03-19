


#pragma once
#ifndef INCLUDED_AI_LOGSTREAM_H
#define INCLUDED_AI_LOGSTREAM_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include "types.h"

namespace Assimp {

class IOSystem;



class ASSIMP_API LogStream
#ifndef SWIG
        : public Intern::AllocateFromAssimpHeap
#endif
{
protected:
    
    LogStream() AI_NO_EXCEPT;

public:
    
    virtual ~LogStream();

    
    
    virtual void write(const char *message) = 0;

    
    
    static LogStream *createDefaultStream(aiDefaultLogStream stream,
            const char *name = "AssimpLog.txt",
            IOSystem *io = nullptr);

}; 

inline LogStream::LogStream() AI_NO_EXCEPT = default;

inline LogStream::~LogStream() = default;

} 

#endif 
