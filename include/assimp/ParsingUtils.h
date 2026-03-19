


#pragma once
#ifndef AI_PARSING_UTILS_H_INC
#define AI_PARSING_UTILS_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/StringComparison.h>
#include <assimp/StringUtils.h>
#include <assimp/defs.h>

#include <vector>
#include <algorithm>

namespace Assimp {










static const unsigned int BufferSize = 4096;


template <class char_t>
AI_FORCE_INLINE bool IsUpper(char_t in) {
    return (in >= (char_t)'A' && in <= (char_t)'Z');
}


template <class char_t>
AI_FORCE_INLINE bool IsLower(char_t in) {
    return (in >= (char_t)'a' && in <= (char_t)'z');
}


template <class char_t>
AI_FORCE_INLINE bool IsSpace(char_t in) {
    return (in == (char_t)' ' || in == (char_t)'\t');
}


template <class char_t>
AI_FORCE_INLINE bool IsLineEnd(char_t in) {
    return (in == (char_t)'\r' || in == (char_t)'\n' || in == (char_t)'\0' || in == (char_t)'\f');
}


template <class char_t>
AI_FORCE_INLINE bool IsSpaceOrNewLine(char_t in) {
    return IsSpace<char_t>(in) || IsLineEnd<char_t>(in);
}


template <class char_t>
AI_FORCE_INLINE bool SkipSpaces(const char_t *in, const char_t **out, const char_t *end) {
    while ((*in == (char_t)' ' || *in == (char_t)'\t') && in != end) {
        ++in;
    }
    *out = in;
    return !IsLineEnd<char_t>(*in);
}


template <class char_t>
AI_FORCE_INLINE bool SkipSpaces(const char_t **inout, const char_t *end) {
    return SkipSpaces<char_t>(*inout, inout, end);
}


template <class char_t>
AI_FORCE_INLINE bool SkipLine(const char_t *in, const char_t **out, const char_t *end) {
    while ((*in != (char_t)'\r' && *in != (char_t)'\n' && *in != (char_t)'\0') && *in != (char_t)'#' && in != end) {
        ++in;
    }

    
    while ((*in == (char_t)'\r' || *in == (char_t)'\n') && in != end) {
        ++in;
    }
    *out = in;
    return *in != (char_t)'\0';
}


template <class char_t>
AI_FORCE_INLINE bool SkipLine(const char_t **inout, const char_t *end) {
    return SkipLine<char_t>(*inout, inout, end);
}


template <class char_t>
AI_FORCE_INLINE bool SkipSpacesAndLineEnd(const char_t *in, const char_t **out, const char_t  *end) {
    while ((*in == (char_t)' ' || *in == (char_t)'\t' || *in == (char_t)'\r' || *in == (char_t)'\n') && in != end) {
        ++in;
    }
    *out = in;
    return *in != '\0';
}


template <class char_t>
AI_FORCE_INLINE bool SkipSpacesAndLineEnd(const char_t **inout, const char_t *end) {
    return SkipSpacesAndLineEnd<char_t>(*inout, inout, end);
}


template <class char_t>
AI_FORCE_INLINE bool GetNextLine(const char_t *&buffer, char_t out[BufferSize]) {
    if ((char_t)'\0' == *buffer) {
        return false;
    }

    char *_out = out;
    char *const end = _out + BufferSize - 1;
    while (!IsLineEnd(*buffer) && _out < end) {
        *_out++ = *buffer++;
    }
    *_out = (char_t)'\0';

    while (IsLineEnd(*buffer) && '\0' != *buffer && buffer != end) {
        ++buffer;
    }

    return true;
}


template <class char_t>
AI_FORCE_INLINE bool IsNumeric(char_t in) {
    return (in >= '0' && in <= '9') || '-' == in || '+' == in;
}


template <class char_t>
AI_FORCE_INLINE bool TokenMatch(char_t *&in, const char *token, unsigned int len) {
    if (!::strncmp(token, in, len) && IsSpaceOrNewLine(in[len])) {
        if (in[len] != '\0') {
            in += len + 1;
        } else {
            
            in += len;
        }
        return true;
    }

    return false;
}


AI_FORCE_INLINE bool TokenMatchI(const char *&in, const char *token, unsigned int len) {
    if (!ASSIMP_strincmp(token, in, len) && IsSpaceOrNewLine(in[len])) {
        in += len + 1;
        return true;
    }
    return false;
}


AI_FORCE_INLINE void SkipToken(const char *&in, const char *end) {
    SkipSpaces(&in, end);
    while (!IsSpaceOrNewLine(*in)) {
        ++in;
    }
}


AI_FORCE_INLINE std::string GetNextToken(const char *&in, const char *end) {
    SkipSpacesAndLineEnd(&in, end);
    const char *cur = in;
    while (!IsSpaceOrNewLine(*in)) {
        ++in;
    }
    return std::string(cur, (size_t)(in - cur));
}



template <class string_type>
AI_FORCE_INLINE unsigned int tokenize(const string_type &str, std::vector<string_type> &tokens,
        const string_type &delimiters) {
    
    typename string_type::size_type lastPos = str.find_first_not_of(delimiters, 0);

    
    typename string_type::size_type pos = str.find_first_of(delimiters, lastPos);
    while (string_type::npos != pos || string_type::npos != lastPos) {
        
        string_type tmp = str.substr(lastPos, pos - lastPos);
        if (!tmp.empty() && ' ' != tmp[0])
            tokens.push_back(tmp);

        
        lastPos = str.find_first_not_of(delimiters, pos);

        
        pos = str.find_first_of(delimiters, lastPos);
    }

    return static_cast<unsigned int>(tokens.size());
}

inline std::string ai_stdStrToLower(const std::string &str) {
    std::string out(str);
    for (size_t i = 0; i < str.size(); ++i) {
        out[i] = (char) tolower((unsigned char)out[i]);
    }
    return out;
}

} 

#endif 
