


#pragma once
#ifndef AI_POSTPROCESS_H_INC
#define AI_POSTPROCESS_H_INC

#include <assimp/types.h>

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif




enum aiPostProcessSteps
{

    
    
    aiProcess_CalcTangentSpace = 0x1,

    
    
    aiProcess_JoinIdenticalVertices = 0x2,

    
    
    aiProcess_MakeLeftHanded = 0x4,

    
    
    aiProcess_Triangulate = 0x8,

    
    
    aiProcess_RemoveComponent = 0x10,

    
    
    aiProcess_GenNormals = 0x20,

    
    
    aiProcess_GenSmoothNormals = 0x40,

    
    
    aiProcess_SplitLargeMeshes = 0x80,

    
    
    aiProcess_PreTransformVertices = 0x100,

    
    
    aiProcess_LimitBoneWeights = 0x200,

    
    
    aiProcess_ValidateDataStructure = 0x400,

    
    
    aiProcess_ImproveCacheLocality = 0x800,

    
    
    aiProcess_RemoveRedundantMaterials = 0x1000,

    
    
    aiProcess_FixInfacingNormals = 0x2000,



    
    
    aiProcess_PopulateArmatureData = 0x4000,

    
    
    aiProcess_SortByPType = 0x8000,

    
    
    aiProcess_FindDegenerates = 0x10000,

    
    
    aiProcess_FindInvalidData = 0x20000,

    
    
    aiProcess_GenUVCoords = 0x40000,

    
    
    aiProcess_TransformUVCoords = 0x80000,

    
    
    aiProcess_FindInstances = 0x100000,

    
    
    aiProcess_OptimizeMeshes  = 0x200000,


    
    
    aiProcess_OptimizeGraph  = 0x400000,

    
    
    aiProcess_FlipUVs = 0x800000,

    
    
    aiProcess_FlipWindingOrder  = 0x1000000,

    
    
    aiProcess_SplitByBoneCount  = 0x2000000,

    
    
    aiProcess_Debone  = 0x4000000,



    
    
    aiProcess_GlobalScale = 0x8000000,

    
    
    aiProcess_EmbedTextures  = 0x10000000,

    
    
    


    aiProcess_ForceGenNormals = 0x20000000,

    
    
    aiProcess_DropNormals = 0x40000000,

    
    
    aiProcess_GenBoundingBoxes = 0x80000000
};




#define aiProcess_ConvertToLeftHanded ( \
    aiProcess_MakeLeftHanded     | \
    aiProcess_FlipUVs            | \
    aiProcess_FlipWindingOrder   | \
    0 )




#define aiProcessPreset_TargetRealtime_Fast ( \
    aiProcess_CalcTangentSpace      |  \
    aiProcess_GenNormals            |  \
    aiProcess_JoinIdenticalVertices |  \
    aiProcess_Triangulate           |  \
    aiProcess_GenUVCoords           |  \
    aiProcess_SortByPType           |  \
    0 )

 
 
#define aiProcessPreset_TargetRealtime_Quality ( \
    aiProcess_CalcTangentSpace              |  \
    aiProcess_GenSmoothNormals              |  \
    aiProcess_JoinIdenticalVertices         |  \
    aiProcess_ImproveCacheLocality          |  \
    aiProcess_LimitBoneWeights              |  \
    aiProcess_RemoveRedundantMaterials      |  \
    aiProcess_SplitLargeMeshes              |  \
    aiProcess_Triangulate                   |  \
    aiProcess_GenUVCoords                   |  \
    aiProcess_SortByPType                   |  \
    aiProcess_FindDegenerates               |  \
    aiProcess_FindInvalidData               |  \
    0 )

 
 
#define aiProcessPreset_TargetRealtime_MaxQuality ( \
    aiProcessPreset_TargetRealtime_Quality   |  \
    aiProcess_FindInstances                  |  \
    aiProcess_ValidateDataStructure          |  \
    aiProcess_OptimizeMeshes                 |  \
    0 )


#ifdef __cplusplus
} 
#endif

#endif 
