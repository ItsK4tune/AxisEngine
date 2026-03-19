

#pragma once
#ifndef AI_IOSTREAMBUFFER_H_INC
#define AI_IOSTREAMBUFFER_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/ParsingUtils.h>
#include <assimp/types.h>
#include <assimp/IOStream.hpp>

#include <vector>

namespace Assimp {



template <class T>
class IOStreamBuffer {
public:
    
    IOStreamBuffer(size_t cache = 4096 * 4096);

    
    ~IOStreamBuffer() = default;

    
    
    
    bool open(IOStream *stream);

    
    
    bool close();

    
    
    size_t size() const;

    
    
    size_t cacheSize() const;

    
    
    bool readNextBlock();

    
    
    size_t getNumBlocks() const;

    
    
    size_t getCurrentBlockIndex() const;

    
    
    size_t getFilePos() const;

    
    
    
    bool getNextDataLine(std::vector<T> &buffer, T continuationToken);

    
    
    
    bool getNextLine(std::vector<T> &buffer);

    
    
    
    bool getNextBlock(std::vector<T> &buffer);

private:
    IOStream *m_stream;
    size_t m_filesize;
    size_t m_cacheSize;
    size_t m_numBlocks;
    size_t m_blockIdx;
    std::vector<T> m_cache;
    size_t m_cachePos;
    size_t m_filePos;
};

template <class T>
AI_FORCE_INLINE IOStreamBuffer<T>::IOStreamBuffer(size_t cache) :
        m_stream(nullptr),
        m_filesize(0),
        m_cacheSize(cache),
        m_numBlocks(0),
        m_blockIdx(0),
        m_cachePos(0),
        m_filePos(0) {
    m_cache.resize(cache);
    std::fill(m_cache.begin(), m_cache.end(), '\n');
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::open(IOStream *stream) {
    
    if (nullptr != m_stream) {
        return false;
    }

    
    if (nullptr == stream) {
        return false;
    }

    m_stream = stream;
    m_filesize = m_stream->FileSize();
    if (m_filesize == 0) {
        return false;
    }
    if (m_filesize < m_cacheSize) {
        m_cacheSize = m_filesize;
    }

    m_numBlocks = m_filesize / m_cacheSize;
    if ((m_filesize % m_cacheSize) > 0) {
        m_numBlocks++;
    }

    return true;
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::close() {
    if (nullptr == m_stream) {
        return false;
    }

    
    m_stream = nullptr;
    m_filesize = 0;
    m_numBlocks = 0;
    m_blockIdx = 0;
    m_cachePos = 0;
    m_filePos = 0;

    return true;
}

template <class T>
AI_FORCE_INLINE
        size_t
        IOStreamBuffer<T>::size() const {
    return m_filesize;
}

template <class T>
AI_FORCE_INLINE
        size_t
        IOStreamBuffer<T>::cacheSize() const {
    return m_cacheSize;
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::readNextBlock() {
    m_stream->Seek(m_filePos, aiOrigin_SET);
    size_t readLen = m_stream->Read(&m_cache[0], sizeof(T), m_cacheSize);
    if (readLen == 0) {
        return false;
    }
    if (readLen < m_cacheSize) {
        m_cacheSize = readLen;
    }
    m_filePos += m_cacheSize;
    m_cachePos = 0;
    m_blockIdx++;

    return true;
}

template <class T>
AI_FORCE_INLINE size_t IOStreamBuffer<T>::getNumBlocks() const {
    return m_numBlocks;
}

template <class T>
AI_FORCE_INLINE size_t IOStreamBuffer<T>::getCurrentBlockIndex() const {
    return m_blockIdx;
}

template <class T>
AI_FORCE_INLINE size_t IOStreamBuffer<T>::getFilePos() const {
    return m_filePos;
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::getNextDataLine(std::vector<T> &buffer, T continuationToken) {
    buffer.resize(m_cacheSize);
    if (m_cachePos >= m_cacheSize || 0 == m_filePos) {
        if (!readNextBlock()) {
            return false;
        }
    }

    size_t i = 0;
    for (;;) {
        if (continuationToken == m_cache[m_cachePos] && IsLineEnd(m_cache[m_cachePos + 1])) {
            ++m_cachePos;
            while (m_cache[m_cachePos] != '\n') {
                ++m_cachePos;
            }
            ++m_cachePos;
        } else if (IsLineEnd(m_cache[m_cachePos])) {
            break;
        }

        buffer[i] = m_cache[m_cachePos];
        ++m_cachePos;
        ++i;

        if(i == buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }

        if (m_cachePos >= size()) {
            break;
        }
        if (m_cachePos >= m_cacheSize) {
            if (!readNextBlock()) {
                return false;
            }
        }
    }

    buffer[i] = '\n';
    ++m_cachePos;

    return true;
}

static AI_FORCE_INLINE bool isEndOfCache(size_t pos, size_t cacheSize) {
    return (pos == cacheSize);
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::getNextLine(std::vector<T> &buffer) {
    buffer.resize(m_cacheSize);
    if (m_cachePos >= m_cacheSize || 0 == m_filePos) {
        if (!readNextBlock()) {
            return false;
        }
    }

    if (IsLineEnd(m_cache[m_cachePos])) {
        
        do {
            ++m_cachePos;
            if (isEndOfCache(m_cachePos, m_cacheSize) && !readNextBlock()) {
                return false;
            }
        }
        while (m_cache[m_cachePos] != '\n');
    }

    size_t i(0);
    while (!IsLineEnd(m_cache[m_cachePos])) {
        buffer[i] = m_cache[m_cachePos];
        ++m_cachePos;
        ++i;

        if(i == buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }

        if (m_cachePos >= m_cacheSize) {
            if (!readNextBlock()) {
                return false;
            }
        }
    }
    buffer[i] = '\n';
    if (m_cachePos < m_cacheSize && (m_cache[m_cachePos] == '\r')) {
        ++m_cachePos;
    }
    if (m_cachePos < m_cacheSize && (m_cache[m_cachePos] == '\n')) {
        ++m_cachePos;
    }

    return true;
}

template <class T>
AI_FORCE_INLINE bool IOStreamBuffer<T>::getNextBlock(std::vector<T> &buffer) {
    
    if (0 != m_cachePos) {
        buffer = std::vector<T>(m_cache.begin() + m_cachePos, m_cache.end());
        m_cachePos = 0;
    } else {
        if (!readNextBlock()) {
            return false;
        }

        buffer = std::vector<T>(m_cache.begin(), m_cache.end());
    }

    return true;
}

} 

#endif 
