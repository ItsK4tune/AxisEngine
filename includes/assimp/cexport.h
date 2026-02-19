


#pragma once
#ifndef AI_EXPORT_H_INC
#define AI_EXPORT_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifndef ASSIMP_BUILD_NO_EXPORT

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;
struct aiFileIO;



struct aiExportFormatDesc {
    
    
    
    const char *id;

    
    
    const char *description;

    
    const char *fileExtension;
};



ASSIMP_API size_t aiGetExportFormatCount(void);



ASSIMP_API const C_STRUCT aiExportFormatDesc *aiGetExportFormatDescription(size_t pIndex);



ASSIMP_API void aiReleaseExportFormatDescription(const C_STRUCT aiExportFormatDesc *desc);



ASSIMP_API void aiCopyScene(const C_STRUCT aiScene *pIn,
        C_STRUCT aiScene **pOut);



ASSIMP_API void aiFreeScene(const C_STRUCT aiScene *pIn);



ASSIMP_API aiReturn aiExportScene(const C_STRUCT aiScene *pScene,
        const char *pFormatId,
        const char *pFileName,
        unsigned int pPreprocessing);



ASSIMP_API aiReturn aiExportSceneEx(const C_STRUCT aiScene *pScene,
        const char *pFormatId,
        const char *pFileName,
        C_STRUCT aiFileIO *pIO,
        unsigned int pPreprocessing);



struct aiExportDataBlob {
    
    size_t size;

    
    void *data;

    
    C_STRUCT aiString name;

    
    C_STRUCT aiExportDataBlob *next;

#ifdef __cplusplus
    
    aiExportDataBlob() {
        size = 0;
        data = next = nullptr;
    }
    
    ~aiExportDataBlob() {
        delete[] static_cast<unsigned char *>(data);
        delete next;
    }

    aiExportDataBlob(const aiExportDataBlob &) = delete;
    aiExportDataBlob &operator=(const aiExportDataBlob &) = delete;

#endif 
};



ASSIMP_API const C_STRUCT aiExportDataBlob *aiExportSceneToBlob(const C_STRUCT aiScene *pScene, const char *pFormatId,
        unsigned int pPreprocessing);



ASSIMP_API void aiReleaseExportBlob(const C_STRUCT aiExportDataBlob *pData);

#ifdef __cplusplus
}
#endif

#endif 
#endif 
