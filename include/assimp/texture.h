


#pragma once
#ifndef AI_TEXTURE_H_INC
#define AI_TEXTURE_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif




#ifndef AI_EMBEDDED_TEXNAME_PREFIX
#   define AI_EMBEDDED_TEXNAME_PREFIX	"*"
#endif


#if (!defined AI_MAKE_EMBEDDED_TEXNAME)
#   define AI_MAKE_EMBEDDED_TEXNAME(_n_) AI_EMBEDDED_TEXNAME_PREFIX # _n_
#endif

#include "./Compiler/pushpack1.h"



struct aiTexel {
    unsigned char b,g,r,a;

#ifdef __cplusplus
    
    bool operator== (const aiTexel& other) const {
        return b == other.b && r == other.r &&
               g == other.g && a == other.a;
    }

    
    bool operator!= (const aiTexel& other) const {
        return b != other.b || r != other.r ||
               g != other.g || a != other.a;
    }

    
    operator aiColor4D() const {
        return aiColor4D(r/255.f,g/255.f,b/255.f,a/255.f);
    }
#endif 

} PACK_STRUCT;

#include "./Compiler/poppack1.h"

#define HINTMAXTEXTURELEN 9



struct aiTexture {
    
    unsigned int mWidth;

    
    unsigned int mHeight;

    
    char achFormatHint[ HINTMAXTEXTURELEN ];

    
    C_STRUCT aiTexel* pcData;

    
    C_STRUCT aiString mFilename;

#ifdef __cplusplus

    
    
    
    
    
    bool CheckFormat(const char* s) const {
        if (nullptr == s) {
            return false;
        }

		return (0 == ::strncmp(achFormatHint, s, sizeof(achFormatHint)));
    }

    
    aiTexture() AI_NO_EXCEPT :
            mWidth(0),
            mHeight(0),
            pcData(nullptr),
            mFilename() {
        memset(achFormatHint, 0, sizeof(achFormatHint));
    }

    
    ~aiTexture () {
        delete[] pcData;
    }
#endif
};


#ifdef __cplusplus
}
#endif

#endif 
