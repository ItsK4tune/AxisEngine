



#ifndef AI_OBJMATERIAL_H_INC
#define AI_OBJMATERIAL_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/material.h>




#define AI_MATKEY_OBJ_ILLUM "$mat.illum", 0, 0








#define _AI_MATKEY_OBJ_BUMPMULT_BASE "$tex.bumpmult"



#define AI_MATKEY_OBJ_BUMPMULT(type, N) _AI_MATKEY_OBJ_BUMPMULT_BASE, type, N


#define AI_MATKEY_OBJ_BUMPMULT_NORMALS(N) \
    AI_MATKEY_OBJ_BUMPMULT(aiTextureType_NORMALS, N)

#define AI_MATKEY_OBJ_BUMPMULT_HEIGHT(N) \
    AI_MATKEY_OBJ_BUMPMULT(aiTextureType_HEIGHT, N)




#endif
