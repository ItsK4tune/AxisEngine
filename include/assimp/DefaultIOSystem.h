


#pragma once
#ifndef AI_DEFAULTIOSYSTEM_H_INC
#define AI_DEFAULTIOSYSTEM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/IOSystem.hpp>

namespace Assimp {



class ASSIMP_API DefaultIOSystem : public IOSystem {
public:
    
    
    bool Exists( const char* pFile) const override;

    
    
    char getOsSeparator() const override;

    
    
    IOStream* Open( const char* pFile, const char* pMode = "rb") override;

    
    
    void Close( IOStream* pFile) override;

    
    
    bool ComparePaths (const char* one, const char* second) const override;

    
    static std::string fileName( const std::string &path );

    
    static std::string completeBaseName( const std::string &path);

    
    static std::string absolutePath( const std::string &path);
};

} 

#endif 
