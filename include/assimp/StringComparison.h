


#pragma once
#ifndef INCLUDED_AI_STRING_WORKERS_H
#define INCLUDED_AI_STRING_WORKERS_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/ai_assert.h>
#include <assimp/defs.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace Assimp {



inline unsigned int ASSIMP_itoa10(char *out, unsigned int max, int32_t number) {
    ai_assert(nullptr != out);

    
    unsigned int written = 1u;
    if (number < 0 && written < max) {
        *out++ = '-';
        ++written;
        number = -number;
    }

    
    int32_t cur = 1000000000; 
    bool mustPrint = false;
    while (written < max) {

        const unsigned int digit = number / cur;
        if (mustPrint || digit > 0 || 1 == cur) {
            
            mustPrint = true;

            *out++ = '0' + static_cast<char>(digit);

            ++written;
            number -= digit * cur;
            if (1 == cur) {
                break;
            }
        }
        cur /= 10;
    }

    
    *out++ = '\0';
    return written - 1;
}



template <size_t length>
inline unsigned int ASSIMP_itoa10(char (&out)[length], int32_t number) {
    return ASSIMP_itoa10(out, length, number);
}



inline int ASSIMP_stricmp(const char *s1, const char *s2) {
    ai_assert(nullptr != s1);
    ai_assert(nullptr != s2);

#if (defined _MSC_VER)

    return ::_stricmp(s1, s2);
#else
    char c1, c2;
    do {
        c1 = tolower((unsigned char)*(s1++));
        c2 = tolower((unsigned char)*(s2++));
    } while (c1 && (c1 == c2));
    return c1 - c2;
#endif
}



inline int ASSIMP_stricmp(const std::string &a, const std::string &b) {
    int i = (int)b.length() - (int)a.length();
    return (i ? i : ASSIMP_stricmp(a.c_str(), b.c_str()));
}



inline int ASSIMP_strincmp(const char *s1, const char *s2, unsigned int n) {
    ai_assert(nullptr != s1);
    ai_assert(nullptr != s2);
    if (!n) {
        return 0;
    }

#if (defined _MSC_VER)

    return ::_strnicmp(s1, s2, n);

#elif defined(__GNUC__)

    return ::strncasecmp(s1, s2, n);

#else
    char c1, c2;
    unsigned int p = 0;
    do {
        if (p++ >= n) return 0;
        c1 = tolower((unsigned char)*(s1++));
        c2 = tolower((unsigned char)*(s2++));
    } while (c1 && (c1 == c2));

    return c1 - c2;
#endif
}



inline unsigned int integer_pow(unsigned int base, unsigned int power) {
    unsigned int res = 1;
    for (unsigned int i = 0; i < power; ++i) {
        res *= base;
    }

    return res;
}

} 

#endif 
