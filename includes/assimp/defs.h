



#pragma once
#ifndef AI_DEFINES_H_INC
#define AI_DEFINES_H_INC

#ifdef __GNUC__
#  pragma GCC system_header
#endif

#include <assimp/config.h>





#ifndef ASSIMP_BUILD_NO_COMPRESSED_X
#  define ASSIMP_BUILD_NEED_Z_INFLATE
#endif

#ifndef ASSIMP_BUILD_NO_COMPRESSED_BLEND
#  define ASSIMP_BUILD_NEED_Z_INFLATE
#endif

#ifndef ASSIMP_BUILD_NO_COMPRESSED_IFC
#  define ASSIMP_BUILD_NEED_Z_INFLATE
#  define ASSIMP_BUILD_NEED_UNZIP
#endif

#ifndef ASSIMP_BUILD_NO_Q3BSP_IMPORTER
#  define ASSIMP_BUILD_NEED_Z_INFLATE
#  define ASSIMP_BUILD_NEED_UNZIP
#endif


#if (!defined SIZE_MAX)
#  define SIZE_MAX (~((size_t)0))
#endif







#ifdef _WIN32
#  undef ASSIMP_API
#  ifdef ASSIMP_BUILD_DLL_EXPORT
#    define ASSIMP_API __declspec(dllexport)
#    define ASSIMP_API_WINONLY __declspec(dllexport)
#  elif (defined ASSIMP_DLL)
#    define ASSIMP_API __declspec(dllimport)
#    define ASSIMP_API_WINONLY __declspec(dllimport)
#  else
#    define ASSIMP_API
#    define ASSIMP_API_WINONLY
#  endif
#else
#  define ASSIMP_API __attribute__((visibility("default")))
#  define ASSIMP_API_WINONLY
#endif 


#ifdef _MSC_VER
    #pragma warning(disable : 4521 4512 4714 4127 4510)
    #if _MSC_VER < 1900
        #pragma warning(disable : 4351)
    #endif
    #ifdef ASSIMP_BUILD_DLL_EXPORT
        #pragma warning(disable : 4251)
    #endif
    #define AI_FORCE_INLINE inline
    #define AI_WONT_RETURN __declspec(noreturn)
#elif defined(SWIG)
  
#else
    #define AI_WONT_RETURN
    #define AI_FORCE_INLINE inline
#endif 

#ifdef __GNUC__
#   define AI_WONT_RETURN_SUFFIX __attribute__((noreturn))
#elif _MSC_VER
#if defined(__clang__)
#   define AI_WONT_RETURN_SUFFIX __attribute__((noreturn))
#else
#   define AI_WONT_RETURN_SUFFIX
#endif
#else
#   define AI_WONT_RETURN_SUFFIX
#endif 

#ifdef __cplusplus

#define C_STRUCT
#define C_ENUM
#else



#if 0
    ENABLE_PREPROCESSING   = YES
    MACRO_EXPANSION        = YES
    EXPAND_ONLY_PREDEF     = YES
    SEARCH_INCLUDES        = YES
    INCLUDE_PATH           =
    INCLUDE_FILE_PATTERNS  =
    PREDEFINED             = ASSIMP_DOXYGEN_BUILD=1
    EXPAND_AS_DEFINED      = C_STRUCT C_ENUM
    SKIP_FUNCTION_MACROS   = YES
#endif



#if (defined ASSIMP_DOXYGEN_BUILD)
#  define C_STRUCT
#  define C_ENUM
#else
#  define C_STRUCT struct
#  define C_ENUM enum
#endif
#endif

#if (defined(__BORLANDC__) || defined(__BCPLUSPLUS__))
#  error Currently, Borland is unsupported. Feel free to port Assimp.
#endif




#ifndef ASSIMP_BUILD_SINGLETHREADED
#  define ASSIMP_BUILD_SINGLETHREADED
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
#  define ASSIMP_BUILD_DEBUG
#endif





#ifdef ASSIMP_DOUBLE_PRECISION
typedef double ai_real;
typedef signed long long int ai_int;
typedef unsigned long long int ai_uint;
#ifndef ASSIMP_AI_REAL_TEXT_PRECISION
#define ASSIMP_AI_REAL_TEXT_PRECISION 17
#endif 
#else 
typedef float ai_real;
typedef signed int ai_int;
typedef unsigned int ai_uint;
#ifndef ASSIMP_AI_REAL_TEXT_PRECISION
#define ASSIMP_AI_REAL_TEXT_PRECISION 9
#endif 
#endif 






#define AI_MATH_PI (3.141592653589793238462643383279)
#define AI_MATH_TWO_PI (AI_MATH_PI * 2.0)
#define AI_MATH_HALF_PI (AI_MATH_PI * 0.5)


#define AI_MATH_PI_F (3.1415926538f)
#define AI_MATH_TWO_PI_F (AI_MATH_PI_F * 2.0f)
#define AI_MATH_HALF_PI_F (AI_MATH_PI_F * 0.5f)


#define AI_DEG_TO_RAD(x) ((x) * (ai_real) 0.0174532925)
#define AI_RAD_TO_DEG(x) ((x) * (ai_real) 57.2957795)


#ifdef __cplusplus
constexpr ai_real ai_epsilon = (ai_real) 1e-6;
#else
#  define ai_epsilon ((ai_real)1e-6)
#endif


#if defined(__BYTE_ORDER__)
#  if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    if !defined(__BIG_ENDIAN__)
#      define __BIG_ENDIAN__
#    endif
#  else 
#    if defined(__BIG_ENDIAN__)
#      undef __BIG_ENDIAN__
#    endif
#  endif
#endif
#if defined(__BIG_ENDIAN__)
#  define AI_BUILD_BIG_ENDIAN
#endif


#define AI_MAX_ALLOC(type) ((256U * 1024 * 1024) / sizeof(type))

#ifndef _MSC_VER
#  if __cplusplus >= 201103L 
#    define AI_NO_EXCEPT noexcept
#  else
#    define AI_NO_EXCEPT
#  endif
#else
#  if (_MSC_VER >= 1915)
#    define AI_NO_EXCEPT noexcept
#  else
#    define AI_NO_EXCEPT
#  endif
#endif 


#if (defined ASSIMP_BUILD_DEBUG)
#  define AI_DEBUG_INVALIDATE_PTR(x) x = NULL;
#else
#  define AI_DEBUG_INVALIDATE_PTR(x)
#endif

#define AI_COUNT_OF(X) (sizeof(X) / sizeof((X)[0]))


#if defined(__GNUC__) || defined(__clang__)
#  define AI_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#  define AI_DEPRECATED __declspec(deprecated)
#else
#  pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#  define AI_DEPRECATED
#endif

#endif 
