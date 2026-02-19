


#pragma once
#ifndef AI_IMPORTER_DESC_H_INC
#define AI_IMPORTER_DESC_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif


enum aiImporterFlags {
    
    aiImporterFlags_SupportTextFlavour = 0x1,

    
    aiImporterFlags_SupportBinaryFlavour = 0x2,

    
    aiImporterFlags_SupportCompressedFlavour = 0x4,

    
    aiImporterFlags_LimitedSupport = 0x8,

    
    aiImporterFlags_Experimental = 0x10
};


struct aiImporterDesc {
    
    const char *mName;

    
    const char *mAuthor;

    
    const char *mMaintainer;

    
    const char *mComments;

    
    unsigned int mFlags;

    
    unsigned int mMinMajor;
    unsigned int mMinMinor;

    
    unsigned int mMaxMajor;
    unsigned int mMaxMinor;

    
    const char *mFileExtensions;
};


ASSIMP_API const C_STRUCT aiImporterDesc *aiGetImporterDesc(const char *extension);

#ifdef __cplusplus
} 
#endif

#endif 
