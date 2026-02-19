


#pragma once
#ifndef AI_VERSION_H_INC
#define AI_VERSION_H_INC

#include <assimp/defs.h>

#ifdef __cplusplus
extern "C" {
#endif



ASSIMP_API const char*  aiGetLegalString  (void);



ASSIMP_API unsigned int aiGetVersionPatch(void);



ASSIMP_API unsigned int aiGetVersionMinor (void);



ASSIMP_API unsigned int aiGetVersionMajor (void);



ASSIMP_API unsigned int aiGetVersionRevision (void);



ASSIMP_API const char *aiGetBranchName();


#define ASSIMP_CFLAGS_SHARED  0x1

#define ASSIMP_CFLAGS_STLPORT 0x2

#define ASSIMP_CFLAGS_DEBUG   0x4


#define ASSIMP_CFLAGS_NOBOOST           0x8

#define ASSIMP_CFLAGS_SINGLETHREADED    0x10

#define ASSIMP_CFLAGS_DOUBLE_SUPPORT 0x20



ASSIMP_API unsigned int aiGetCompileFlags(void);

#ifdef __cplusplus
} 
#endif

#endif 

