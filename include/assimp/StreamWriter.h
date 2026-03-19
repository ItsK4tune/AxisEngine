


#pragma once
#ifndef AI_STREAMWRITER_H_INCLUDED
#define AI_STREAMWRITER_H_INCLUDED

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/ByteSwapper.h>
#include <assimp/IOStream.hpp>

#include <memory>
#include <vector>

namespace Assimp {




template <bool SwapEndianness = false, bool RuntimeSwitch = false>
class StreamWriter {
    enum {
        INITIAL_CAPACITY = 1024
    };

public:

    
    
    StreamWriter(std::shared_ptr<IOStream> stream, bool le = false)
        : stream(stream)
        , le(le)
        , cursor()
    {
        ai_assert(stream);
        buffer.reserve(INITIAL_CAPACITY);
    }

    
    StreamWriter(IOStream* stream, bool le = false)
        : stream(std::shared_ptr<IOStream>(stream))
        , le(le)
        , cursor()
    {
        ai_assert(stream);
        buffer.reserve(INITIAL_CAPACITY);
    }

    
    ~StreamWriter() {
        stream->Write(buffer.data(), 1, buffer.size());
        stream->Flush();
    }

public:

    
    
    void Flush()
    {
        stream->Write(buffer.data(), 1, buffer.size());
        stream->Flush();
        buffer.clear();
        cursor = 0;
    }

    
    
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin=aiOrigin_SET)
    {
        Flush();
        return stream->Seek(pOffset, pOrigin);
    }

    
    
    size_t Tell()
    {
        Flush();
        return stream->Tell();
    }

public:

    
    
    void PutF4(float f)
    {
        Put(f);
    }

    
    
    void PutF8(double d)    {
        Put(d);
    }

    
    
    void PutI2(int16_t n)   {
        Put(n);
    }

    
    
    void PutI1(int8_t n)    {
        Put(n);
    }

    
    
    void PutI4(int32_t n)   {
        Put(n);
    }

    
    
    void PutI8(int64_t n)   {
        Put(n);
    }

    
    
    void PutU2(uint16_t n)  {
        Put(n);
    }

    
    
    void PutU1(uint8_t n)   {
        Put(n);
    }

    
    
    void PutU4(uint32_t n)  {
        Put(n);
    }

    
    
    void PutU8(uint64_t n)  {
        Put(n);
    }

    
    
    void PutChar(char c)    {
        Put(c);
    }

    
    
    void PutString(const aiString& s)
    {
        
        if (cursor + s.length >= buffer.size()) {
            buffer.resize(cursor + s.length);
        }
        void* dest = &buffer[cursor];
        ::memcpy(dest, s.C_Str(), s.length);
        cursor += s.length;
    }

    
    
    void PutString(const std::string& s)
    {
        
        if (cursor + s.size() >= buffer.size()) {
            buffer.resize(cursor + s.size());
        }
        void* dest = &buffer[cursor];
        ::memcpy(dest, s.c_str(), s.size());
        cursor += s.size();
    }

public:

    
    
    template <typename T>
    StreamWriter& operator << (T f) {
        Put(f);
        return *this;
    }

    
    std::size_t GetCurrentPos() const {
        return cursor;
    }

    
    void SetCurrentPos(std::size_t new_cursor) {
        cursor = new_cursor;
    }

    
    
    template <typename T>
    void Put(T f)   {
        Intern :: Getter<SwapEndianness,T,RuntimeSwitch>() (&f, le);

        if (cursor + sizeof(T) >= buffer.size()) {
            buffer.resize(cursor + sizeof(T));
        }

        void* dest = &buffer[cursor];

        
        
        
        ::memcpy(dest, &f, sizeof(T));
        cursor += sizeof(T);
    }

private:

    std::shared_ptr<IOStream> stream;
    bool le;

    std::vector<uint8_t> buffer;
    std::size_t cursor;
};




#ifdef AI_BUILD_BIG_ENDIAN
    typedef StreamWriter<true>  StreamWriterLE;
    typedef StreamWriter<false> StreamWriterBE;
#else
    typedef StreamWriter<true>  StreamWriterBE;
    typedef StreamWriter<false> StreamWriterLE;
#endif



typedef StreamWriter<true,true> StreamWriterAny;

} 

#endif 
