
#pragma once
#ifndef INCLUDED_AI_STRINGUTILS_H
#define INCLUDED_AI_STRINGUTILS_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/defs.h>

#include <cstdarg>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <locale>
#include <sstream>
#include <iomanip>

#if defined(_MSC_VER) && !defined(__clang__)
#  define AI_SIZEFMT "%Iu"
#else
#  define AI_SIZEFMT "%zu"
#endif












#if defined(_MSC_VER) && _MSC_VER < 1900

inline int c99_ai_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap) {
    int count(-1);
    if (0 != size) {
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    }
    if (count == -1) {
        count = _vscprintf(format, ap);
    }

    return count;
}

inline int ai_snprintf(char *outBuf, size_t size, const char *format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_ai_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#elif defined(__MINGW32__)
#  define ai_snprintf __mingw_snprintf
#else
#  define ai_snprintf snprintf
#endif








template <typename T>
AI_FORCE_INLINE std::string ai_to_string(T value) {
    std::ostringstream os;
    os << value;

    return os.str();
}








AI_FORCE_INLINE
float ai_strtof(const char *begin, const char *end) {
    if (nullptr == begin) {
        return 0.0f;
    }
    float val(0.0f);
    if (nullptr == end) {
        val = static_cast<float>(::atof(begin));
    } else {
        std::string::size_type len(end - begin);
        std::string token(begin, len);
        val = static_cast<float>(::atof(token.c_str()));
    }

    return val;
}







template <class T>
AI_FORCE_INLINE std::string ai_decimal_to_hexa(T toConvert) {
    std::string result;
    std::stringstream ss;
    ss << std::hex << toConvert;
    ss >> result;

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = (char)toupper((unsigned char)result[i]);
    }

    return result;
}










AI_FORCE_INLINE std::string ai_rgba2hex(int r, int g, int b, int a, bool with_head) {
    std::stringstream ss;
    if (with_head) {
        ss << "#";
    }
    ss << std::hex << std::setfill('0') << std::setw(8) << (r << 24 | g << 16 | b << 8 | a);

    return ss.str();
}





AI_FORCE_INLINE void ai_trim_left(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}





AI_FORCE_INLINE void ai_trim_right(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}





AI_FORCE_INLINE std::string ai_trim(std::string &s) {
    std::string out(s);
    ai_trim_left(out);
    ai_trim_right(out);

    return out;
}






template <class char_t>
AI_FORCE_INLINE char_t ai_tolower(char_t in) {
    return (in >= (char_t)'A' && in <= (char_t)'Z') ? (char_t)(in + 0x20) : in;
}






AI_FORCE_INLINE std::string ai_tolower(const std::string &in) {
    std::string out(in);
    ai_trim_left(out);
    ai_trim_right(out);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return ai_tolower(c); });
    return out;
}






template <class char_t>
AI_FORCE_INLINE char_t ai_toupper(char_t in) {
    return (in >= (char_t)'a' && in <= (char_t)'z') ? (char_t)(in - 0x20) : in;
}






AI_FORCE_INLINE std::string ai_str_toupper(const std::string &in) {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [](char c) { return ai_toupper(c); });
    return out;
}








AI_FORCE_INLINE std::string ai_str_toprintable(const std::string &in, char placeholder = '?') {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [placeholder] (unsigned char c) {
        return isprint(c) ? (char)c :  placeholder;
    });
    return out;
}










AI_FORCE_INLINE std::string ai_str_toprintable(const char *in, int len, char placeholder = '?') {
    return (in && len > 0) ? ai_str_toprintable(std::string(in, len), placeholder) : std::string();
}

#endif 
