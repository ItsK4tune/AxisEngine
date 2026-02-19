





#pragma once
#ifndef AI_SKELETONMESHBUILDER_H_INC
#define AI_SKELETONMESHBUILDER_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/mesh.h>
#include <vector>

struct aiMaterial;
struct aiScene;
struct aiNode;

namespace Assimp {



class ASSIMP_API SkeletonMeshBuilder {
public:
    
    
    SkeletonMeshBuilder(aiScene *pScene, aiNode *root = nullptr,
            bool bKnobsOnly = false);

protected:
    
    
    void CreateGeometry(const aiNode *pNode);

    
    
    aiMesh *CreateMesh();

    
    
    aiMaterial *CreateMaterial();

private:
    
    std::vector<aiVector3D> mVertices;

    
    struct Face {
        unsigned int mIndices[3];
        Face();
        Face(unsigned int p0, unsigned int p1, unsigned int p2) {
            mIndices[0] = p0;
            mIndices[1] = p1;
            mIndices[2] = p2;
        }
    };
    std::vector<Face> mFaces;

    
    std::vector<aiBone *> mBones;

    bool mKnobsOnly;
};

} 

#endif 
