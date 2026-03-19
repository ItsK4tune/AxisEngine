


#pragma once
#ifndef AI_STREAMREADER_H_INCLUDED
#define AI_STREAMREADER_H_INCLUDED

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/ByteSwapper.h>
#include <assimp/Exceptional.h>
#include <assimp/IOStream.hpp>

#include <memory>

namespace Assimp {




template <bool SwapEndianness = false, bool RuntimeSwitch = false>
class StreamReader {
public:
    using diff = size_t;
    using pos = size_t;

    
    
    StreamReader(std::shared_ptr<IOStream> stream, bool le = false) :
            mStream(stream),
            mBuffer(nullptr),
            mCurrent(nullptr),
            mEnd(nullptr),
            mLimit(nullptr),
            mLe(le) {
        ai_assert(stream);
        InternBegin();
    }

    
    StreamReader(IOStream *stream, bool le = false) :
            mStream(std::shared_ptr<IOStream>(stream)),
            mBuffer(nullptr),
            mCurrent(nullptr),
            mEnd(nullptr),
            mLimit(nullptr),
            mLe(le) {
        ai_assert(nullptr != stream);
        InternBegin();
    }

    
    ~StreamReader() {
        delete[] mBuffer;
    }

    

    
    
    float GetF4() {
        return Get<float>();
    }

    
    
    double GetF8() {
        return Get<double>();
    }

    
    
    int16_t GetI2() {
        return Get<int16_t>();
    }

    
    
    int8_t GetI1() {
        return Get<int8_t>();
    }

    
    
    int32_t GetI4() {
        return Get<int32_t>();
    }

    
    
    int64_t GetI8() {
        return Get<int64_t>();
    }

    
    
    uint16_t GetU2() {
        return Get<uint16_t>();
    }

    
    
    uint8_t GetU1() {
        return Get<uint8_t>();
    }

    
    
    uint32_t GetU4() {
        return Get<uint32_t>();
    }

    
    
    uint64_t GetU8() {
        return Get<uint64_t>();
    }

    
    
    size_t GetRemainingSize() const {
        return (unsigned int)(mEnd - mCurrent);
    }

    
    
    size_t GetRemainingSizeToLimit() const {
        return (unsigned int)(mLimit - mCurrent);
    }

    
    
    void IncPtr(intptr_t plus) {
        mCurrent += plus;
        if (mCurrent > mLimit) {
            throw DeadlyImportError("End of file or read limit was reached");
        }
    }

    
    
    int8_t *GetPtr() const {
        return mCurrent;
    }

    
    
    void SetPtr(int8_t *p) {
        mCurrent = p;
        if (mCurrent > mLimit || mCurrent < mBuffer) {
            throw DeadlyImportError("End of file or read limit was reached");
        }
    }

    
    
    void CopyAndAdvance(void *out, size_t bytes) {
        int8_t *ur = GetPtr();
        SetPtr(ur + bytes); 

        ::memcpy(out, ur, bytes);
    }

    
    int GetCurrentPos() const {
        return (unsigned int)(mCurrent - mBuffer);
    }

    void SetCurrentPos(size_t pos) {
        SetPtr(mBuffer + pos);
    }

    
    
    unsigned int SetReadLimit(unsigned int _limit) {
        unsigned int prev = GetReadLimit();
        if (UINT_MAX == _limit) {
            mLimit = mEnd;
            return prev;
        }

        mLimit = mBuffer + _limit;
        if (mLimit > mEnd) {
            throw DeadlyImportError("StreamReader: Invalid read limit");
        }
        return prev;
    }

    
    
    unsigned int GetReadLimit() const {
        return (unsigned int)(mLimit - mBuffer);
    }

    
    
    void SkipToReadLimit() {
        mCurrent = mLimit;
    }

    
    
    template <typename T>
    StreamReader &operator>>(T &f) {
        f = Get<T>();
        return *this;
    }

    
    
    template <typename T>
    T Get() {
        if (mCurrent + sizeof(T) > mLimit) {
            throw DeadlyImportError("End of file or stream limit was reached");
        }

        T f;
        ::memcpy(&f, mCurrent, sizeof(T));
        Intern::Getter<SwapEndianness, T, RuntimeSwitch>()(&f, mLe);
        mCurrent += sizeof(T);

        return f;
    }

private:
    
    void InternBegin() {
        if (nullptr == mStream) {
            throw DeadlyImportError("StreamReader: Unable to open file");
        }

        const size_t filesize = mStream->FileSize() - mStream->Tell();
        if (0 == filesize) {
            throw DeadlyImportError("StreamReader: File is empty or EOF is already reached");
        }

        mCurrent = mBuffer = new int8_t[filesize];
        const size_t read = mStream->Read(mCurrent, 1, filesize);
        
        ai_assert(read <= filesize);
        mEnd = mLimit = &mBuffer[read - 1] + 1;
    }

private:
    std::shared_ptr<IOStream> mStream;
    int8_t *mBuffer;
    int8_t *mCurrent;
    int8_t *mEnd;
    int8_t *mLimit;
    bool mLe;
};



#ifdef AI_BUILD_BIG_ENDIAN
typedef StreamReader<true> StreamReaderLE;
typedef StreamReader<false> StreamReaderBE;
#else
typedef StreamReader<true> StreamReaderBE;
typedef StreamReader<false> StreamReaderLE;
#endif



typedef StreamReader<true, true> StreamReaderAny;

} 

#endif 
