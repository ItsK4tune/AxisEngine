



#ifndef AVUTIL_AVASSERT_H
#define AVUTIL_AVASSERT_H

#include <stdlib.h>
#ifdef HAVE_AV_CONFIG_H
#   include "config.h"
#endif
#include "attributes.h"
#include "log.h"
#include "macros.h"


#define av_assert0(cond) do {                                           \
    if (!(cond)) {                                                      \
        av_log(NULL, AV_LOG_PANIC, "Assertion %s failed at %s:%d\n",    \
               AV_STRINGIFY(cond), __FILE__, __LINE__);                 \
        abort();                                                        \
    }                                                                   \
} while (0)



#if defined(ASSERT_LEVEL) && ASSERT_LEVEL > 0
#define av_assert1(cond) av_assert0(cond)
#else
#define av_assert1(cond) ((void)0)
#endif



#if defined(ASSERT_LEVEL) && ASSERT_LEVEL > 1
#define av_assert2(cond) av_assert0(cond)
#define av_assert2_fpu() av_assert0_fpu()
#else
#define av_assert2(cond) ((void)0)
#define av_assert2_fpu() ((void)0)
#endif


void av_assert0_fpu(void);


#if defined(ASSERT_LEVEL) ? ASSERT_LEVEL > 0 : !defined(HAVE_AV_CONFIG_H) && !defined(NDEBUG)
#define av_unreachable(msg)                                             \
do {                                                                    \
    av_log(NULL, AV_LOG_PANIC,                                          \
           "Reached supposedly unreachable code at %s:%d: %s\n",        \
           __FILE__, __LINE__, msg);                                    \
    abort();                                                            \
} while (0)
#define av_assume(cond) av_assert0(cond)
#else
#if AV_GCC_VERSION_AT_LEAST(4, 5) || AV_HAS_BUILTIN(__builtin_unreachable)
#define av_unreachable(msg) __builtin_unreachable()
#elif  defined(_MSC_VER)
#define av_unreachable(msg) __assume(0)
#elif __STDC_VERSION__ >= 202311L
#include <stddef.h>
#define av_unreachable(msg) unreachable()
#else
#define av_unreachable(msg) ((void)0)
#endif

#if AV_HAS_BUILTIN(__builtin_assume)
#define av_assume(cond) __builtin_assume(cond)
#elif defined(_MSC_VER)
#define av_assume(cond) __assume(cond)
#else
#define av_assume(cond) do { \
    if (!(cond))             \
        av_unreachable();    \
} while (0)
#endif
#endif

#endif 
