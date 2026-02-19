



#pragma once
#ifndef AI_ZIPARCHIVEIOSYSTEM_H_INC
#define AI_ZIPARCHIVEIOSYSTEM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <zlib.h>

namespace Assimp {

class ZipArchiveIOSystem : public IOSystem {
public:
    
    ZipArchiveIOSystem(IOSystem* pIOHandler, const char *pFilename, const char* pMode = "r");
    ZipArchiveIOSystem(IOSystem* pIOHandler, const std::string& rFilename, const char* pMode = "r");
    ~ZipArchiveIOSystem() override;
    bool Exists(const char* pFilename) const override;
    char getOsSeparator() const override;
    IOStream* Open(const char* pFilename, const char* pMode = "rb") override;
    void Close(IOStream* pFile) override;

    
    
    bool isOpen() const;

    
    
    void getFileList(std::vector<std::string>& rFileList) const;

    
    
    void getFileListExtension(std::vector<std::string>& rFileList, const std::string& extension) const;

    static bool isZipArchive(IOSystem* pIOHandler, const char *pFilename);
    static bool isZipArchive(IOSystem* pIOHandler, const std::string& rFilename);

private:
    class Implement;
    Implement *pImpl = nullptr;
};

} 

#endif 
