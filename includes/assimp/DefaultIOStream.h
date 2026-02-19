


#pragma once
#ifndef AI_DEFAULTIOSTREAM_H_INC
#define AI_DEFAULTIOSTREAM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <cstdio>
#include <assimp/IOStream.hpp>
#include <assimp/importerdesc.h>

namespace Assimp {







class ASSIMP_API DefaultIOStream : public IOStream {
    friend class DefaultIOSystem;
#if __ANDROID__
# if __ANDROID_API__ > 9
#  if defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)
    friend class AndroidJNIIOSystem;
#  endif 
# endif 
#endif 

protected:
    
    DefaultIOStream() AI_NO_EXCEPT;

    
    
    
    DefaultIOStream(FILE* pFile, const std::string &strFilename);

public:
    
    ~DefaultIOStream () override;

    
    
    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override;

    
    
    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override;

    
    
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;

    
    
    size_t Tell() const override;

    
    
    size_t FileSize() const override;

    
    
    void Flush() override;

private:
    FILE* mFile;
    std::string mFilename;
    mutable size_t mCachedSize;
};


AI_FORCE_INLINE DefaultIOStream::DefaultIOStream() AI_NO_EXCEPT :
        mFile(nullptr),
        mFilename(),
        mCachedSize(SIZE_MAX) {
    
}


AI_FORCE_INLINE DefaultIOStream::DefaultIOStream (FILE* pFile, const std::string &strFilename) :
        mFile(pFile),
        mFilename(strFilename),
        mCachedSize(SIZE_MAX) {
    
}



} 

#endif 
