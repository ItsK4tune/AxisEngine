

#pragma once
#ifndef AI_INCLUDED_EXCEPTIONAL_H
#define AI_INCLUDED_EXCEPTIONAL_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/DefaultIOStream.h>
#include <assimp/TinyFormatter.h>
#include <stdexcept>

using std::runtime_error;

#ifdef _MSC_VER
#pragma warning(disable : 4275)
#endif



class ASSIMP_API DeadlyErrorBase : public runtime_error {
protected:
    
    
    DeadlyErrorBase(Assimp::Formatter::format f);

    
    
    
    
    
    
    template<typename... T, typename U>
    DeadlyErrorBase(Assimp::Formatter::format f, U&& u, T&&... args) :
            DeadlyErrorBase(std::move(f << std::forward<U>(u)), std::forward<T>(args)...) {}
};



class ASSIMP_API DeadlyImportError : public DeadlyErrorBase {
public:
    
    
    DeadlyImportError(const char *message) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<const char*>(message)) {
        
    }

    
    
    
    template<typename... T>
    explicit DeadlyImportError(T&&... args) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<T>(args)...) {
        
    }
};



class ASSIMP_API DeadlyExportError : public DeadlyErrorBase {
public:
    
    template<typename... T>
    explicit DeadlyExportError(T&&... args) :
            DeadlyErrorBase(Assimp::Formatter::format(), std::forward<T>(args)...) {}
};

#ifdef _MSC_VER
#pragma warning(default : 4275)
#endif


template <typename T>
struct ExceptionSwallower {
    T operator()() const {
        return T();
    }
};


template <typename T>
struct ExceptionSwallower<T *> {
    T *operator()() const {
        return nullptr;
    }
};


template <>
struct ExceptionSwallower<aiReturn> {
    aiReturn operator()() const {
        try {
            throw;
        } catch (std::bad_alloc &) {
            return aiReturn_OUTOFMEMORY;
        } catch (...) {
            return aiReturn_FAILURE;
        }
    }
};


template <>
struct ExceptionSwallower<void> {
    void operator()() const {
        return;
    }
};

#define ASSIMP_BEGIN_EXCEPTION_REGION() \
    {                                   \
        try {

#define ASSIMP_END_EXCEPTION_REGION_WITH_ERROR_STRING(type, ASSIMP_END_EXCEPTION_REGION_errorString, ASSIMP_END_EXCEPTION_REGION_exception)     \
    }                                                                                                                                           \
    catch (const DeadlyImportError &e) {                                                                                                        \
        ASSIMP_END_EXCEPTION_REGION_errorString = e.what();                                                                                     \
        ASSIMP_END_EXCEPTION_REGION_exception = std::current_exception();                                                                       \
        return ExceptionSwallower<type>()();                                                                                                    \
    }                                                                                                                                           \
    catch (...) {                                                                                                                               \
        ASSIMP_END_EXCEPTION_REGION_errorString = "Unknown exception";                                                                          \
        ASSIMP_END_EXCEPTION_REGION_exception = std::current_exception();                                                                       \
        return ExceptionSwallower<type>()();                                                                                                    \
    }                                                                                                                                           \
}

#define ASSIMP_END_EXCEPTION_REGION(type)    \
    }                                        \
    catch (...) {                            \
        return ExceptionSwallower<type>()(); \
    }                                        \
    }

#endif 
