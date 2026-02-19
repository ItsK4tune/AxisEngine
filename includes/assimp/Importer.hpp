


#pragma once
#ifndef AI_ASSIMP_HPP_INC
#define AI_ASSIMP_HPP_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifndef __cplusplus
#error This header requires C++ to be used. Use assimp.h for plain C.
#endif 


#include <assimp/types.h>

#include <exception>

namespace Assimp {


class Importer;
class IOStream;
class IOSystem;
class ProgressHandler;







class BaseImporter;
class BaseProcess;
class SharedPostProcessInfo;
class BatchLoader;



class ImporterPimpl;
} 

#define AI_PROPERTY_WAS_NOT_EXISTING 0xffffffff

struct aiScene;


struct aiImporterDesc;


namespace Assimp {



class ASSIMP_API Importer {
public:
    
    static const unsigned int MaxLenHint = 200;

public:
    
    
    Importer();

    
    
    Importer(const Importer &other) = delete;

    
    
    Importer &operator=(const Importer &) = delete;

    
    
    ~Importer();

    
    
    aiReturn RegisterLoader(BaseImporter *pImp);

    
    
    aiReturn UnregisterLoader(BaseImporter *pImp);

    
    
    aiReturn RegisterPPStep(BaseProcess *pImp);

    
    
    aiReturn UnregisterPPStep(BaseProcess *pImp);

    
    
    bool SetPropertyInteger(const char *szName, int iValue);

    
    
    bool SetPropertyBool(const char *szName, bool value) {
        return SetPropertyInteger(szName, value);
    }

    
    
    bool SetPropertyFloat(const char *szName, ai_real fValue);

    
    
    bool SetPropertyString(const char *szName, const std::string &sValue);

    
    
    bool SetPropertyMatrix(const char *szName, const aiMatrix4x4 &sValue);

    
    
    bool SetPropertyPointer(const char *szName, void *sValue);

    
    
    int GetPropertyInteger(const char *szName,
            int iErrorReturn = 0xffffffff) const;

    
    
    bool GetPropertyBool(const char *szName, bool bErrorReturn = false) const {
        return GetPropertyInteger(szName, bErrorReturn) != 0;
    }

    
    
    ai_real GetPropertyFloat(const char *szName,
            ai_real fErrorReturn = 10e10) const;

    
    
    std::string GetPropertyString(const char *szName,
            const std::string &sErrorReturn = std::string()) const;

    
    
    aiMatrix4x4 GetPropertyMatrix(const char *szName,
            const aiMatrix4x4 &sErrorReturn = aiMatrix4x4()) const;

    
    
    void* GetPropertyPointer(const char *szName,
        void *sErrorReturn = nullptr) const;

    
    
    void SetIOHandler(IOSystem *pIOHandler);

    
    
    IOSystem *GetIOHandler() const;

    
    
    bool IsDefaultIOHandler() const;

    
    
    void SetProgressHandler(ProgressHandler *pHandler);

    
    
    ProgressHandler *GetProgressHandler() const;

    
    
    bool IsDefaultProgressHandler() const;

    
    
    bool ValidateFlags(unsigned int pFlags) const;

    
    
    const aiScene *ReadFile(
            const char *pFile,
            unsigned int pFlags);

    
    
    const aiScene *ReadFileFromMemory(
            const void *pBuffer,
            size_t pLength,
            unsigned int pFlags,
            const char *pHint = "");

    
    
    const aiScene *ApplyPostProcessing(unsigned int pFlags);

    const aiScene *ApplyCustomizedPostProcessing(BaseProcess *rootProcess, bool requestValidation);

    
    
    const aiScene *ReadFile(
            const std::string &pFile,
            unsigned int pFlags);

    
    
    void FreeScene();

    
    
    const char *GetErrorString() const;

    
    
    const std::exception_ptr& GetException() const;

    
    
    const aiScene *GetScene() const;

    
    
    aiScene *GetOrphanedScene();

    
    
    bool IsExtensionSupported(const char *szExtension) const;

    
    
    inline bool IsExtensionSupported(const std::string &szExtension) const;

    
    
    void GetExtensionList(aiString &szOut) const;

    
    
    inline void GetExtensionList(std::string &szOut) const;

    
    
    size_t GetImporterCount() const;

    
    
    const aiImporterDesc *GetImporterInfo(size_t index) const;

    
    
    BaseImporter *GetImporter(size_t index) const;

    
    
    BaseImporter *GetImporter(const char *szExtension) const;

    
    
    size_t GetImporterIndex(const char *szExtension) const;

    
    
    void GetMemoryRequirements(aiMemoryInfo &in) const;

    
    
    void SetExtraVerbose(bool bDo);

    
    
    ImporterPimpl *Pimpl() { return pimpl; }
    const ImporterPimpl *Pimpl() const { return pimpl; }

protected:
    
    ImporterPimpl *pimpl;
}; 








AI_FORCE_INLINE const aiScene *Importer::ReadFile(const std::string &pFile, unsigned int pFlags) {
    return ReadFile(pFile.c_str(), pFlags);
}

AI_FORCE_INLINE void Importer::GetExtensionList(std::string &szOut) const {
    aiString s;
    GetExtensionList(s);
    szOut = s.data;
}

AI_FORCE_INLINE bool Importer::IsExtensionSupported(const std::string &szExtension) const {
    return IsExtensionSupported(szExtension.c_str());
}

} 

#endif 
