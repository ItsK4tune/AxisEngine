

#pragma once
#ifndef AI_AABB_H_INC
#define AI_AABB_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/vector3.h>



struct aiAABB {
    C_STRUCT aiVector3D mMin;
    C_STRUCT aiVector3D mMax;

#ifdef __cplusplus
    
    aiAABB() = default;

    
    
    
    aiAABB(const aiVector3D &min, const aiVector3D &max) : mMin(min), mMax(max) {}

    
    ~aiAABB() = default;

#endif 
};

#endif 
