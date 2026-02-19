


#pragma once
#ifndef AI_MEMORYIOSTREAM_H_INC
#define AI_MEMORYIOSTREAM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/ai_assert.h>

#include <stdint.h>

namespace Assimp {

#define AI_MEMORYIO_MAGIC_FILENAME "$$$___magic___$$$"
#define AI_MEMORYIO_MAGIC_FILENAME_LENGTH 17




class MemoryIOStream : public IOStream {
public:
    MemoryIOStream (const uint8_t* buff, size_t len, bool own = false) :
            buffer (buff),
            length(len),
            pos(static_cast<size_t>(0)),
            own(own) {
        
    }

    ~MemoryIOStream() override  {
        if(own) {
            delete[] buffer;
        }
    }

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        ai_assert(nullptr != pvBuffer);
        ai_assert(0 != pSize);

        const size_t cnt = std::min( pCount, (length-pos) / pSize);
        const size_t ofs = pSize * cnt;

        ::memcpy(pvBuffer,buffer+pos,ofs);
        pos += ofs;

        return cnt;
    }

    size_t Write(const void*, size_t, size_t ) override {
        ai_assert(false); 
        return 0;
    }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        if (aiOrigin_SET == pOrigin) {
            if (pOffset > length) {
                return AI_FAILURE;
            }
            pos = pOffset;
        } else if (aiOrigin_END == pOrigin) {
            if (pOffset > length) {
                return AI_FAILURE;
            }
            pos = length-pOffset;
        } else {
            if (pOffset+pos > length) {
                return AI_FAILURE;
            }
            pos += pOffset;
        }
        return AI_SUCCESS;
    }

    size_t Tell() const override {
        return pos;
    }

    size_t FileSize() const override {
        return length;
    }

    void Flush() override{
        ai_assert(false); 
    }

private:
    const uint8_t* buffer;
    size_t length,pos;
    bool own;
};



class MemoryIOSystem : public IOSystem {
public:
    
    MemoryIOSystem(const uint8_t* buff, size_t len, IOSystem* io) : buffer(buff), length(len), existing_io(io) {
        
    }

    
    ~MemoryIOSystem() override = default;

    
    
    bool Exists(const char* pFile) const override {
        if (0 == strncmp( pFile, AI_MEMORYIO_MAGIC_FILENAME, AI_MEMORYIO_MAGIC_FILENAME_LENGTH ) ) {
            return true;
        }
        return existing_io ? existing_io->Exists(pFile) : false;
    }

    
    
    char getOsSeparator() const override {
        return existing_io ? existing_io->getOsSeparator()
                           : '/';  
    }

    
    
    IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        if ( 0 == strncmp( pFile, AI_MEMORYIO_MAGIC_FILENAME, AI_MEMORYIO_MAGIC_FILENAME_LENGTH ) ) {
            created_streams.emplace_back(new MemoryIOStream(buffer, length));
            return created_streams.back();
        }
        return existing_io ? existing_io->Open(pFile, pMode) : nullptr;
    }

    
    
    void Close( IOStream* pFile) override {
        auto it = std::find(created_streams.begin(), created_streams.end(), pFile);
        if (it != created_streams.end()) {
            delete pFile;
            created_streams.erase(it);
        } else if (existing_io) {
            existing_io->Close(pFile);
        }
    }

    
    
    bool ComparePaths(const char* one, const char* second) const override {
        return existing_io ? existing_io->ComparePaths(one, second) : false;
    }

    
    bool PushDirectory( const std::string &path ) override {
        return existing_io ? existing_io->PushDirectory(path) : false;
    }

    
    const std::string &CurrentDirectory() const override {
        static std::string empty;
        return existing_io ? existing_io->CurrentDirectory() : empty;
    }

    
    size_t StackSize() const override {
        return existing_io ? existing_io->StackSize() : 0;
    }

    
    bool PopDirectory() override {
        return existing_io ? existing_io->PopDirectory() : false;
    }

    
    bool CreateDirectory( const std::string &path ) override {
        return existing_io ? existing_io->CreateDirectory(path) : false;
    }

    
    bool ChangeDirectory( const std::string &path ) override {
        return existing_io ? existing_io->ChangeDirectory(path) : false;
    }

    
    bool DeleteFile( const std::string &file ) override {
        return existing_io ? existing_io->DeleteFile(file) : false;
    }

private:
    const uint8_t* buffer;
    size_t length;
    IOSystem* existing_io;
    std::vector<IOStream*> created_streams;
};

} 

#endif 
