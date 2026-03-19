



#pragma once
#ifndef AI_IOSYSTEM_H_INC
#define AI_IOSYSTEM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#ifndef __cplusplus
#   error This header requires C++ to be used. aiFileIO.h is the \
    corresponding C interface.
#endif

#include "types.h"

#ifdef _WIN32
#   include <direct.h>
#   include <cstdlib>
#   include <cstdio>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <unistd.h>
#endif 

#include <vector>

namespace Assimp {

class IOStream;



class ASSIMP_API IOSystem
#ifndef SWIG
    : public Intern::AllocateFromAssimpHeap
#endif
{
public:

    
    
    IOSystem() AI_NO_EXCEPT = default;

    
    
    virtual ~IOSystem() = default;

    
    
    AI_FORCE_INLINE bool Exists( const std::string& pFile) const;

    
    
    virtual bool Exists( const char* pFile) const = 0;

    
    
    virtual char getOsSeparator() const = 0;

    
    
    virtual IOStream* Open(const char* pFile,
        const char* pMode = "rb") = 0;

    
    
    inline IOStream* Open(const std::string& pFile,
        const std::string& pMode = std::string("rb"));

    
    
    virtual void Close( IOStream* pFile) = 0;

    
    
    virtual bool ComparePaths (const char* one,
        const char* second) const;

    
    
    inline bool ComparePaths (const std::string& one,
        const std::string& second) const;

    
    
    virtual bool PushDirectory( const std::string &path );

    
    
    virtual const std::string &CurrentDirectory() const;

    
    
    virtual size_t StackSize() const;

    
    
    virtual bool PopDirectory();

    
    
    virtual bool CreateDirectory( const std::string &path );

    
    
    virtual bool ChangeDirectory( const std::string &path );

    
    
    virtual bool DeleteFile(const std::string &file);

private:
    std::vector<std::string> m_pathStack;
};








AI_FORCE_INLINE IOStream* IOSystem::Open(const std::string& pFile, const std::string& pMode) {
    
    
    
    return Open(pFile.c_str(),pMode.c_str());
}


AI_FORCE_INLINE bool IOSystem::Exists( const std::string& pFile) const {
    
    
    
    return Exists(pFile.c_str());
}


AI_FORCE_INLINE bool IOSystem::ComparePaths(const std::string& one, const std::string& second) const {
    
    
    
    return ComparePaths(one.c_str(),second.c_str());
}


AI_FORCE_INLINE bool IOSystem::PushDirectory( const std::string &path ) {
    if ( path.empty() ) {
        return false;
    }

    m_pathStack.push_back( path );

    return true;
}


AI_FORCE_INLINE size_t IOSystem::StackSize() const {
    return m_pathStack.size();
}


AI_FORCE_INLINE bool IOSystem::PopDirectory() {
    if ( m_pathStack.empty() ) {
        return false;
    }

    m_pathStack.pop_back();

    return true;
}


AI_FORCE_INLINE bool IOSystem::CreateDirectory( const std::string &path ) {
    if ( path.empty() ) {
        return false;
    }

#ifdef _WIN32
    return 0 != ::_mkdir( path.c_str() );
#else
    return 0 != ::mkdir( path.c_str(), 0777 );
#endif 
}


AI_FORCE_INLINE bool IOSystem::ChangeDirectory( const std::string &path ) {
    if ( path.empty() ) {
        return false;
    }

#ifdef _WIN32
    return 0 != ::_chdir( path.c_str() );
#else
    return 0 != ::chdir( path.c_str() );
#endif 
}



AI_FORCE_INLINE bool IOSystem::DeleteFile( const std::string &file ) {
    if ( file.empty() ) {
        return false;
    }
    const int retCode( ::remove( file.c_str() ) );
    return ( 0 == retCode );
}
} 

#endif 
