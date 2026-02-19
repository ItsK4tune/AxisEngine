

#pragma once
#ifndef AI_GENERIC_PROPERTY_H_INCLUDED
#define AI_GENERIC_PROPERTY_H_INCLUDED

#ifdef __GNUC__
#    pragma GCC system_header
#endif

#include <assimp/Hash.h>
#include <assimp/ai_assert.h>
#include <assimp/Importer.hpp>

#include <map>


template <class T>
inline bool SetGenericProperty(std::map<unsigned int, T> &list,
        const char *szName, const T &value) {
    ai_assert(nullptr != szName);
    const uint32_t hash = SuperFastHash(szName);

    typename std::map<unsigned int, T>::iterator it = list.find(hash);
    if (it == list.end()) {
        list.insert(std::pair<unsigned int, T>(hash, value));
        return false;
    }
    (*it).second = value;

    return true;
}


template <class T>
inline const T &GetGenericProperty(const std::map<unsigned int, T> &list,
        const char *szName, const T &errorReturn) {
    ai_assert(nullptr != szName);
    const uint32_t hash = SuperFastHash(szName);

    typename std::map<unsigned int, T>::const_iterator it = list.find(hash);
    if (it == list.end()) {
        return errorReturn;
    }

    return (*it).second;
}




template <class T>
inline void SetGenericPropertyPtr(std::map<unsigned int, T *> &list,
        const char *szName, T *value, bool *bWasExisting = nullptr) {
    ai_assert(nullptr != szName);
    const uint32_t hash = SuperFastHash(szName);

    typename std::map<unsigned int, T *>::iterator it = list.find(hash);
    if (it == list.end()) {
        if (bWasExisting) {
            *bWasExisting = false;
        }

        list.insert(std::pair<unsigned int, T *>(hash, value));
        return;
    }
    if ((*it).second != value) {
        delete (*it).second;
        (*it).second = value;
    }
    if (!value) {
        list.erase(it);
    }
    if (bWasExisting) {
        *bWasExisting = true;
    }
}


template <class T>
inline bool HasGenericProperty(const std::map<unsigned int, T> &list,
        const char *szName) {
    ai_assert(nullptr != szName);
    const uint32_t hash = SuperFastHash(szName);

    typename std::map<unsigned int, T>::const_iterator it = list.find(hash);
    if (it == list.end()) {
        return false;
    }

    return true;
}

#endif 
