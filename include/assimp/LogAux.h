


#pragma once
#ifndef INCLUDED_AI_LOGAUX_H
#define INCLUDED_AI_LOGAUX_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/TinyFormatter.h>
#include <assimp/Exceptional.h>
#include <assimp/DefaultLogger.hpp>

namespace Assimp {



template<class TDeriving>
class LogFunctions {
public:
    
    template<typename... T>
    static void ThrowException(T&&... args)
    {
        throw DeadlyImportError(Prefix(), std::forward<T>(args)...);
    }

    
    template<typename... T>
    static void LogWarn(T&&... args) {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_WARN(Prefix(), std::forward<T>(args)...);
        }
    }

    
    template<typename... T>
    static void LogError(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_ERROR(Prefix(), std::forward<T>(args)...);
        }
    }

    
    template<typename... T>
    static void LogInfo(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_INFO(Prefix(), std::forward<T>(args)...);
        }
    }

    
    template<typename... T>
    static void LogDebug(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_DEBUG(Prefix(), std::forward<T>(args)...);
        }
    }

    
    template<typename... T>
    static void LogVerboseDebug(T&&... args)  {
        if (!DefaultLogger::isNullLogger()) {
            ASSIMP_LOG_VERBOSE_DEBUG(Prefix(), std::forward<T>(args)...);
        }
    }

private:
    static const char* Prefix();
};

} 

#endif 
