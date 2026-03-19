



#pragma once
#ifndef AI_SMOOTHINGGROUPS_H_INC
#define AI_SMOOTHINGGROUPS_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/vector3.h>

#include <stdint.h>
#include <vector>



struct FaceWithSmoothingGroup {
    FaceWithSmoothingGroup() AI_NO_EXCEPT
    : mIndices()
    , iSmoothGroup(0) {
        
#ifdef ASSIMP_BUILD_DEBUG
        this->mIndices[0] = 0xffffffff;
        this->mIndices[1] = 0xffffffff;
        this->mIndices[2] = 0xffffffff;
#endif
    }


    
    
    
    uint32_t mIndices[3];

    
    uint32_t iSmoothGroup;
};



template <class T>
struct MeshWithSmoothingGroups
{
    
    std::vector<aiVector3D> mPositions;

    
    std::vector<T> mFaces;

    
    std::vector<aiVector3D> mNormals;
};



template <class T>
void ComputeNormalsWithSmoothingsGroups(MeshWithSmoothingGroups<T>& sMesh);



#include "SmoothingGroups.inl"

#endif 
