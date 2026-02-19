


#pragma once
#ifndef INCLUDED_TINY_FORMATTER_H
#define INCLUDED_TINY_FORMATTER_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <sstream>

namespace Assimp {
namespace Formatter {



template < typename T,
    typename CharTraits = std::char_traits<T>,
    typename Allocator  = std::allocator<T> >
class basic_formatter {
public:
    typedef class std::basic_string<T,CharTraits,Allocator> string;
    typedef class std::basic_ostringstream<T,CharTraits,Allocator> stringstream;

    basic_formatter() {
        
    }

    
    template <typename TT>
    basic_formatter(const TT& sin)  {
        underlying << sin;
    }

    
    
#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 9)
    basic_formatter(basic_formatter&& other) {
        underlying << (string)other;
    }
#else
    basic_formatter(basic_formatter&& other)
        : underlying(std::move(other.underlying)) {
    }
#endif

    
    
    
    
    
#if !defined(__GNUC__) || !defined(__APPLE__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    explicit basic_formatter(const basic_formatter& other) {
        underlying << (string)other;
    }
#endif

    operator string () const {
        return underlying.str();
    }

    
    template <typename TToken, typename std::enable_if<!std::is_base_of<std::exception, TToken>::value>::type * = nullptr>
    const basic_formatter &operator<<(const TToken &s) const {
        underlying << s;
        return *this;
    }

    template <typename TToken, typename std::enable_if<std::is_base_of<std::exception, TToken>::value>::type * = nullptr>
    const basic_formatter &operator<<(const TToken &s) const {
        underlying << s.what();
        return *this;
    }

    template <typename TToken, typename std::enable_if<!std::is_base_of<std::exception, TToken>::value>::type * = nullptr>
    basic_formatter &operator<<(const TToken &s) {
        underlying << s;
        return *this;
    }

    template <typename TToken, typename std::enable_if<std::is_base_of<std::exception, TToken>::value>::type * = nullptr>
    basic_formatter &operator<<(const TToken &s) {
        underlying << s.what();
        return *this;
    }


    
    template <typename TToken>
    const basic_formatter& operator, (const TToken& s) const {
        *this << s;
        return *this;
    }

    template <typename TToken>
    basic_formatter& operator, (const TToken& s) {
        *this << s;
        return *this;
    }

    
    
    template <typename TToken>
    basic_formatter& operator, (TToken& s) {
        *this << s;
        return *this;
    }


private:
    mutable stringstream underlying;
};


typedef basic_formatter< char > format;
typedef basic_formatter< wchar_t > wformat;

} 

} 

#endif
