


#pragma once
#ifndef AI_EXPORT_HPP_INC
#define AI_EXPORT_HPP_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifndef ASSIMP_BUILD_NO_EXPORT

#include "cexport.h"
#include <map>
#include <functional>

namespace Assimp {

class ExporterPimpl;
class IOSystem;
class ProgressHandler;



class ASSIMP_API ExportProperties;

class ASSIMP_API Exporter {
public:
    
    typedef void (*fpExportFunc)(const char *, IOSystem *, const aiScene *, const ExportProperties *);

    
    struct ExportFormatEntry {
        
        aiExportFormatDesc mDescription;

        
        fpExportFunc mExportFunction;

        
        unsigned int mEnforcePP;

        
        ExportFormatEntry(const char *pId, const char *pDesc, const char *pExtension, fpExportFunc pFunction, unsigned int pEnforcePP = 0u) {
            mDescription.id = pId;
            mDescription.description = pDesc;
            mDescription.fileExtension = pExtension;
            mExportFunction = pFunction;
            mEnforcePP = pEnforcePP;
        }

        ExportFormatEntry() :
                mExportFunction(),
                mEnforcePP() {
            mDescription.id = nullptr;
            mDescription.description = nullptr;
            mDescription.fileExtension = nullptr;
        }
    };

    
    Exporter();

    
    ~Exporter();

    
    
    void SetIOHandler(IOSystem *pIOHandler);

    
    
    IOSystem *GetIOHandler() const;

    
    
    bool IsDefaultIOHandler() const;

    
    
    void SetProgressHandler(ProgressHandler *pHandler);

    
    
    const aiExportDataBlob *ExportToBlob(const aiScene *pScene, const char *pFormatId,
            unsigned int pPreprocessing = 0u, const ExportProperties *pProperties = nullptr);
    const aiExportDataBlob *ExportToBlob(const aiScene *pScene, const std::string &pFormatId,
            unsigned int pPreprocessing = 0u, const ExportProperties *pProperties = nullptr);

    
    
    aiReturn Export(const aiScene *pScene, const char *pFormatId, const char *pPath,
            unsigned int pPreprocessing = 0u, const ExportProperties *pProperties = nullptr);
    aiReturn Export(const aiScene *pScene, const std::string &pFormatId, const std::string &pPath,
            unsigned int pPreprocessing = 0u, const ExportProperties *pProperties = nullptr);

    
    
    const char *GetErrorString() const;

    
    
    const aiExportDataBlob *GetBlob() const;

    
    
    const aiExportDataBlob *GetOrphanedBlob() const;

    
    
    void FreeBlob();

    
    
    size_t GetExportFormatCount() const;

    
    
    const aiExportFormatDesc *GetExportFormatDescription(size_t pIndex) const;

    
    
    aiReturn RegisterExporter(const ExportFormatEntry &desc);

    
    
    void UnregisterExporter(const char *id);

protected:
    
    ExporterPimpl *pimpl;
};

class ASSIMP_API ExportProperties {
public:
    
    typedef unsigned int KeyType;

    
    
    typedef std::map<KeyType, int> IntPropertyMap;
    typedef std::map<KeyType, ai_real> FloatPropertyMap;
    typedef std::map<KeyType, std::string> StringPropertyMap;
    typedef std::map<KeyType, aiMatrix4x4> MatrixPropertyMap;
    typedef std::map<KeyType, std::function<void *(void *)>> CallbackPropertyMap;

public:
    
    ExportProperties();

    
    
    ExportProperties(const ExportProperties &other);

    
    
    bool SetPropertyInteger(const char *szName, int iValue);

    
    
    bool SetPropertyBool(const char *szName, bool value) {
        return SetPropertyInteger(szName, value);
    }

    
    
    bool SetPropertyFloat(const char *szName, ai_real fValue);

    
    
    bool SetPropertyString(const char *szName, const std::string &sValue);

    
    
    bool SetPropertyMatrix(const char *szName, const aiMatrix4x4 &sValue);

    bool SetPropertyCallback(const char *szName, const std::function<void *(void *)> &f);

    
    
    int GetPropertyInteger(const char *szName,
            int iErrorReturn = 0xffffffff) const;

    
    
    bool GetPropertyBool(const char *szName, bool bErrorReturn = false) const {
        return GetPropertyInteger(szName, bErrorReturn) != 0;
    }

    
    
    ai_real GetPropertyFloat(const char *szName,
            ai_real fErrorReturn = 10e10f) const;

    
    
    const std::string GetPropertyString(const char *szName,
            const std::string &sErrorReturn = "") const;

    
    
    const aiMatrix4x4 GetPropertyMatrix(const char *szName,
            const aiMatrix4x4 &sErrorReturn = aiMatrix4x4()) const;

    std::function<void *(void *)> GetPropertyCallback(const char* szName) const;

    
    
    bool HasPropertyInteger(const char *szName) const;

    
    bool HasPropertyBool(const char *szName) const;

    
    bool HasPropertyFloat(const char *szName) const;

    
    bool HasPropertyString(const char *szName) const;

    
    bool HasPropertyMatrix(const char *szName) const;

    bool HasPropertyCallback(const char *szName) const;

    
    IntPropertyMap mIntProperties;

    
    FloatPropertyMap mFloatProperties;

    
    StringPropertyMap mStringProperties;

    
    MatrixPropertyMap mMatrixProperties;

    CallbackPropertyMap mCallbackProperties;
};


inline const aiExportDataBlob *Exporter::ExportToBlob(const aiScene *pScene, const std::string &pFormatId,
        unsigned int pPreprocessing, const ExportProperties *pProperties) {
    return ExportToBlob(pScene, pFormatId.c_str(), pPreprocessing, pProperties);
}


inline aiReturn Exporter ::Export(const aiScene *pScene, const std::string &pFormatId,
        const std::string &pPath, unsigned int pPreprocessing,
        const ExportProperties *pProperties) {
    return Export(pScene, pFormatId.c_str(), pPath.c_str(), pPreprocessing, pProperties);
}

} 

#endif 
#endif 
