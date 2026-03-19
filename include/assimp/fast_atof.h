#pragma once














#pragma once
#ifndef FAST_A_TO_F_H_INCLUDED
#define FAST_A_TO_F_H_INCLUDED

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <cmath>
#include <limits>
#include <stdint.h>
#include <assimp/defs.h>

#include "StringComparison.h"
#include <assimp/DefaultLogger.hpp>
#include <assimp/Exceptional.h>
#include <assimp/StringUtils.h>

#ifdef _MSC_VER
#  include <stdint.h>
#else
#  include <assimp/Compiler/pstdint.h>
#endif

namespace Assimp {

static constexpr size_t NumItems = 16;

constexpr double fast_atof_table[NumItems] =  {  
    0.0,
    0.1,
    0.01,
    0.001,
    0.0001,
    0.00001,
    0.000001,
    0.0000001,
    0.00000001,
    0.000000001,
    0.0000000001,
    0.00000000001,
    0.000000000001,
    0.0000000000001,
    0.00000000000001,
    0.000000000000001
};




inline unsigned int strtoul10( const char* in, const char** out=0) {
    unsigned int value = 0;

    for ( ;; ) {
        if ( *in < '0' || *in > '9' ) {
            break;
        }

        value = ( value * 10 ) + ( *in - '0' );
        ++in;
    }
    if ( out ) {
        *out = in;
    }
    return value;
}




inline unsigned int strtoul8( const char* in, const char** out=0) {
    unsigned int value( 0 );
    for ( ;; ) {
        if ( *in < '0' || *in > '7' ) {
            break;
        }

        value = ( value << 3 ) + ( *in - '0' );
        ++in;
    }
    if ( out ) {
        *out = in;
    }
    return value;
}




inline unsigned int strtoul16( const char* in, const char** out=0) {
    unsigned int value( 0 );
    for ( ;; ) {
        if ( *in >= '0' && *in <= '9' ) {
            value = ( value << 4u ) + ( *in - '0' );
        } else if (*in >= 'A' && *in <= 'F') {
            value = ( value << 4u ) + ( *in - 'A' ) + 10;
        } else if (*in >= 'a' && *in <= 'f') {
            value = ( value << 4u ) + ( *in - 'a' ) + 10;
        } else {
            break;
        }
        ++in;
    }
    if ( out ) {
        *out = in;
    }
    return value;
}





inline unsigned int HexDigitToDecimal(char in) {
    unsigned int out( UINT_MAX );
    if ( in >= '0' && in <= '9' ) {
        out = in - '0';
    } else if ( in >= 'a' && in <= 'f' ) {
        out = 10u + in - 'a';
    } else if ( in >= 'A' && in <= 'F' ) {
        out = 10u + in - 'A';
    }

    
    return out;
}




inline uint8_t HexOctetToDecimal(const char* in) {
    return ((uint8_t)HexDigitToDecimal(in[0])<<4)+(uint8_t)HexDigitToDecimal(in[1]);
}




inline int strtol10( const char* in, const char** out = 0) {
    bool inv = (*in=='-');
    if ( inv || *in == '+' ) {
        ++in;
    }

    int value = strtoul10(in,out);
    if (inv) {
        if (value < INT_MAX && value > INT_MIN) {
            value = -value;
        } else {
            ASSIMP_LOG_WARN( "Converting the string \"", in, "\" into an inverted value resulted in overflow." );
        }
    }
    return value;
}







inline unsigned int strtoul_cppstyle( const char* in, const char** out=0) {
    if ('0' == in[0]) {
        return 'x' == in[1] ? strtoul16(in+2,out) : strtoul8(in+1,out);
    }
    return strtoul10(in, out);
}





template<typename ExceptionType = DeadlyImportError>
inline uint64_t strtoul10_64( const char* in, const char** out=0, unsigned int* max_inout=0) {
    unsigned int cur = 0;
    uint64_t value = 0;

    if ( *in < '0' || *in > '9' ) {
        
        throw ExceptionType("The string \"", ai_str_toprintable(in, (int)strlen(in)), "\" cannot be converted into a value." );
    }

    for ( ;; ) {
        if ( *in < '0' || *in > '9' ) {
            break;
        }

        const uint64_t new_value = ( value * (uint64_t) 10 ) + ( (uint64_t) ( *in - '0' ) );

        
        if ( new_value < value ) {
            ASSIMP_LOG_WARN( "Converting the string \"", in, "\" into a value resulted in overflow." );
            return 0;
        }

        value = new_value;

        ++in;
        ++cur;

        if (max_inout && *max_inout == cur) {
            if (out) { 
                while ( *in >= '0' && *in <= '9' ) {
                    ++in;
                }
                *out = in;
            }

            return value;
        }
    }
    if ( out ) {
        *out = in;
    }

    if ( max_inout ) {
        *max_inout = cur;
    }

    return value;
}




template<typename ExceptionType = DeadlyImportError>
inline int64_t strtol10_64(const char* in, const char** out = 0, unsigned int* max_inout = 0) {
    bool inv = (*in == '-');
    if ( inv || *in == '+' ) {
        ++in;
    }

    int64_t value = strtoul10_64<ExceptionType>(in, out, max_inout);
    if (inv) {
        value = -value;
    }
    return value;
}


#define AI_FAST_ATOF_RELAVANT_DECIMALS 15






template<typename Real, typename ExceptionType = DeadlyImportError>
inline const char* fast_atoreal_move(const char* c, Real& out, bool check_comma = true) {
    Real f = 0;

    bool inv = (*c == '-');
    if (inv || *c == '+') {
        ++c;
    }

    if ((c[0] == 'N' || c[0] == 'n') && ASSIMP_strincmp(c, "nan", 3) == 0) {
        out = std::numeric_limits<Real>::quiet_NaN();
        c += 3;
        return c;
    }

    if ((c[0] == 'I' || c[0] == 'i') && ASSIMP_strincmp(c, "inf", 3) == 0) {
        out = std::numeric_limits<Real>::infinity();
        if (inv) {
            out = -out;
        }
        c += 3;
        if ((c[0] == 'I' || c[0] == 'i') && ASSIMP_strincmp(c, "inity", 5) == 0) {
            c += 5;
        }
        return c;
     }

    if (!(c[0] >= '0' && c[0] <= '9') &&
            !((c[0] == '.' || (check_comma && c[0] == ',')) && c[1] >= '0' && c[1] <= '9')) {
        
        throw ExceptionType("Cannot parse string \"", ai_str_toprintable(c, (int)strlen(c)),
                                    "\" as a real number: does not start with digit "
                                    "or decimal point followed by digit.");
    }

    if (*c != '.' && (! check_comma || c[0] != ',')) {
        f = static_cast<Real>( strtoul10_64<ExceptionType> ( c, &c) );
    }

    if ((*c == '.' || (check_comma && c[0] == ',')) && c[1] >= '0' && c[1] <= '9') {
        ++c;

        
        
        
        

        
        
        
        unsigned int diff = AI_FAST_ATOF_RELAVANT_DECIMALS;
        double pl = static_cast<double>( strtoul10_64<ExceptionType> ( c, &c, &diff ));

        pl *= fast_atof_table[diff];
        f += static_cast<Real>( pl );
    }
    
    else if (*c == '.') {
        ++c;
    }

    
    
    if (*c == 'e' || *c == 'E') {
        ++c;
        const bool einv = (*c=='-');
        if (einv || *c=='+') {
            ++c;
        }

        
        
        
        Real exp = static_cast<Real>( strtoul10_64<ExceptionType>(c, &c) );
        if (einv) {
            exp = -exp;
        }
        f *= std::pow(static_cast<Real>(10.0), exp);
    }

    if (inv) {
        f = -f;
    }
    out = f;
    return c;
}



template<typename ExceptionType = DeadlyImportError>
inline ai_real fast_atof(const char* c) {
    ai_real ret(0.0);
    fast_atoreal_move<ai_real, ExceptionType>(c, ret);

    return ret;
}

template<typename ExceptionType = DeadlyImportError>
inline
ai_real fast_atof( const char* c, const char** cout) {
    ai_real ret(0.0);
    *cout = fast_atoreal_move<ai_real, ExceptionType>(c, ret);

    return ret;
}

template<typename ExceptionType = DeadlyImportError>
inline ai_real fast_atof( const char** inout) {
    ai_real ret(0.0);
    *inout = fast_atoreal_move<ai_real, ExceptionType>(*inout, ret);

    return ret;
}

} 

#endif 
