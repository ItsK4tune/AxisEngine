


#pragma once
#ifndef AI_IOSTREAM_H_INC
#define AI_IOSTREAM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

#ifndef __cplusplus
#   error This header requires C++ to be used. aiFileIO.h is the \
    corresponding C interface.
#endif

namespace Assimp {



class ASSIMP_API IOStream
#ifndef SWIG
    : public Intern::AllocateFromAssimpHeap
#endif
{
protected:
    
    IOStream() AI_NO_EXCEPT = default;

public:
    
    
    virtual ~IOStream() = default;

    
    
    virtual size_t Read(void* pvBuffer,
        size_t pSize,
        size_t pCount) = 0;

    
    
    virtual size_t Write(const void* pvBuffer,
        size_t pSize,
        size_t pCount) = 0;

    
    
    virtual aiReturn Seek(size_t pOffset,
        aiOrigin pOrigin) = 0;

    
    
    virtual size_t Tell() const = 0;

    
    
    virtual size_t FileSize() const = 0;

    
    
    virtual void Flush() = 0;
}; 

} 

#endif 
