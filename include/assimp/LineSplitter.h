


#pragma once
#ifndef INCLUDED_LINE_SPLITTER_H
#define INCLUDED_LINE_SPLITTER_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <stdexcept>
#include <assimp/StreamReader.h>
#include <assimp/ParsingUtils.h>

namespace Assimp {




class LineSplitter {
public:
    typedef size_t line_idx;

    
    
    LineSplitter(StreamReaderLE& stream, bool skip_empty_lines = true, bool trim = true);

    ~LineSplitter() = default;

    
    
    LineSplitter& operator++();

    
    LineSplitter& operator++(int);

    
    
    const char* operator[] (size_t idx) const;

    
    
    template <size_t N>
    void get_tokens(const char* (&tokens)[N]) const;

    
    
    const std::string* operator -> () const;

    std::string operator* () const;

    const char *getEnd() const;

    
    
    operator bool() const;

    
    
    operator line_idx() const;

    line_idx get_index() const;

    
    
    StreamReaderLE& get_stream();

    
    
    bool match_start(const char* check);

    
    
    void swallow_next_increment();

    LineSplitter( const LineSplitter & ) = delete;
    LineSplitter(LineSplitter &&) = delete;
    LineSplitter &operator = ( const LineSplitter & ) = delete;

private:
    line_idx mIdx;
    std::string mCur;
    const char *mEnd;
    StreamReaderLE& mStream;
    bool mSwallow, mSkip_empty_lines, mTrim;
};

AI_FORCE_INLINE LineSplitter::LineSplitter(StreamReaderLE& stream, bool skip_empty_lines, bool trim ) :
        mIdx(0),
        mCur(),
        mEnd(nullptr),
        mStream(stream),
        mSwallow(),
        mSkip_empty_lines(skip_empty_lines),
        mTrim(trim) {
    mCur.reserve(1024);
    mEnd = mCur.c_str() + 1024;
    operator++();
    mIdx = 0;
}

AI_FORCE_INLINE LineSplitter& LineSplitter::operator++() {
    if (mSwallow) {
        mSwallow = false;
        return *this;
    }

    if (!*this) {
        throw std::logic_error("End of file, no more lines to be retrieved.");
    }

    char s;
    mCur.clear();
    while (mStream.GetRemainingSize() && (s = mStream.GetI1(), 1)) {
        if (s == '\n' || s == '\r') {
            if (mSkip_empty_lines) {
                while (mStream.GetRemainingSize() && ((s = mStream.GetI1()) == ' ' || s == '\r' || s == '\n'));
                if (mStream.GetRemainingSize()) {
                    mStream.IncPtr(-1);
                }
            } else {
                
                if (mStream.GetRemainingSize() && (s == '\r' && mStream.GetI1() != '\n')) {
                    mStream.IncPtr(-1);
                }
                if (mTrim) {
                    while (mStream.GetRemainingSize() && ((s = mStream.GetI1()) == ' ' || s == '\t'));
                    if (mStream.GetRemainingSize()) {
                        mStream.IncPtr(-1);
                    }
                }
            }
            break;
        }
        mCur += s;
    }
    ++mIdx;

    return *this;
}

AI_FORCE_INLINE LineSplitter &LineSplitter::operator++(int) {
    return ++(*this);
}

AI_FORCE_INLINE const char *LineSplitter::operator[] (size_t idx) const {
    const char* s = operator->()->c_str();

    SkipSpaces(&s, mEnd);
    for (size_t i = 0; i < idx; ++i) {
        for (; !IsSpace(*s); ++s) {
            if (IsLineEnd(*s)) {
                throw std::range_error("Token index out of range, EOL reached");
            }
        }
        SkipSpaces(&s, mEnd);
    }
    return s;
}

template <size_t N>
AI_FORCE_INLINE void LineSplitter::get_tokens(const char* (&tokens)[N]) const {
    const char* s = operator->()->c_str();

    SkipSpaces(&s, mEnd);
    for (size_t i = 0; i < N; ++i) {
        if (IsLineEnd(*s)) {
            throw std::range_error("Token count out of range, EOL reached");
        }
        tokens[i] = s;

        for (; *s && !IsSpace(*s); ++s);
        SkipSpaces(&s, mEnd);
    }
}

AI_FORCE_INLINE const std::string* LineSplitter::operator -> () const {
    return &mCur;
}

AI_FORCE_INLINE std::string LineSplitter::operator* () const {
    return mCur;
}

AI_FORCE_INLINE const char* LineSplitter::getEnd() const {
    return mEnd;
}

AI_FORCE_INLINE LineSplitter::operator bool() const {
    return mStream.GetRemainingSize() > 0;
}

AI_FORCE_INLINE LineSplitter::operator line_idx() const {
    return mIdx;
}

AI_FORCE_INLINE LineSplitter::line_idx LineSplitter::get_index() const {
    return mIdx;
}

AI_FORCE_INLINE StreamReaderLE &LineSplitter::get_stream() {
    return mStream;
}

AI_FORCE_INLINE bool LineSplitter::match_start(const char* check) {
    const size_t len = ::strlen(check);

    return len <= mCur.length() && std::equal(check, check + len, mCur.begin());
}

AI_FORCE_INLINE void LineSplitter::swallow_next_increment() {
    mSwallow = true;
}

} 

#endif 
