#pragma once

#include <core/logic/logger.h>
#include <cassert>

#ifdef _DEBUG
#define AXIS_ASSERT(condition, message)                                                                              \
    do                                                                                                               \
    {                                                                                                                \
        if (!(condition))                                                                                            \
        {                                                                                                            \
            LOGGER_ERROR("ASSERT") << "Assertion failed: (" #condition "), " << message << " at " << __FILE__ << ":" \
                                   << __LINE__;                                                                      \
            assert(condition);                                                                                       \
        }                                                                                                            \
    } while (false)
#else
#define AXIS_ASSERT(condition, message)                                \
    do                                                                 \
    {                                                                  \
        if (!(condition))                                              \
        {                                                              \
            LOGGER_ERROR("ASSERT") << "Assertion failed: " << message; \
        }                                                              \
    } while (false)
#endif

#define AXIS_VERIFY(condition, message)                                                                          \
    do                                                                                                           \
    {                                                                                                            \
        if (!(condition))                                                                                        \
        {                                                                                                        \
            LOGGER_ERROR("VERIFY") << "Verification failed: (" #condition "), " << message << " at " << __FILE__ \
                                   << ":" << __LINE__;                                                           \
        }                                                                                                        \
    } while (false)
