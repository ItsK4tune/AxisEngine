


#pragma once
#ifndef AI_MESH_H_INC
#define AI_MESH_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(disable : 4351)
#endif 

#include <assimp/aabb.h>
#include <assimp/types.h>

#ifdef __cplusplus
#include <unordered_set>

extern "C" {
#endif









#ifndef AI_MAX_FACE_INDICES
#define AI_MAX_FACE_INDICES 0x7fff
#endif



#ifndef AI_MAX_BONE_WEIGHTS
#define AI_MAX_BONE_WEIGHTS 0x7fffffff
#endif



#ifndef AI_MAX_VERTICES
#define AI_MAX_VERTICES 0x7fffffff
#endif



#ifndef AI_MAX_FACES
#define AI_MAX_FACES 0x7fffffff
#endif



#ifndef AI_MAX_NUMBER_OF_COLOR_SETS
#define AI_MAX_NUMBER_OF_COLOR_SETS 0x8
#endif 



#ifndef AI_MAX_NUMBER_OF_TEXTURECOORDS
#define AI_MAX_NUMBER_OF_TEXTURECOORDS 0x8
#endif 



struct aiFace {
    
    
    unsigned int mNumIndices;

    
    unsigned int *mIndices;

#ifdef __cplusplus

    
    aiFace() AI_NO_EXCEPT
            : mNumIndices(0),
              mIndices(nullptr) {
        
    }

    
    ~aiFace() {
        delete[] mIndices;
    }

    
    aiFace(const aiFace &o) :
            mNumIndices(0), mIndices(nullptr) {
        *this = o;
    }

    
    aiFace &operator=(const aiFace &o) {
        if (&o == this) {
            return *this;
        }

        delete[] mIndices;
        mNumIndices = o.mNumIndices;
        if (mNumIndices) {
            mIndices = new unsigned int[mNumIndices];
            ::memcpy(mIndices, o.mIndices, mNumIndices * sizeof(unsigned int));
        } else {
            mIndices = nullptr;
        }

        return *this;
    }

    
    bool operator==(const aiFace &o) const {
        if (mIndices == o.mIndices) {
            return true;
        }

        if (nullptr != mIndices && mNumIndices != o.mNumIndices) {
            return false;
        }

        if (nullptr == mIndices) {
            return false;
        }

        for (unsigned int i = 0; i < this->mNumIndices; ++i) {
            if (mIndices[i] != o.mIndices[i]) {
                return false;
            }
        }

        return true;
    }

    
    
    bool operator!=(const aiFace &o) const {
        return !(*this == o);
    }
#endif 
}; 



struct aiVertexWeight {
    
    unsigned int mVertexId;

    
    
    ai_real mWeight;

#ifdef __cplusplus

    
    aiVertexWeight() AI_NO_EXCEPT
            : mVertexId(0),
              mWeight(0.0f) {
        
    }

    
    
    
    aiVertexWeight(unsigned int pID, float pWeight) :
            mVertexId(pID), mWeight(pWeight) {
        
    }

    bool operator==(const aiVertexWeight &rhs) const {
        return (mVertexId == rhs.mVertexId && mWeight == rhs.mWeight);
    }

    bool operator!=(const aiVertexWeight &rhs) const {
        return (*this == rhs);
    }

#endif 
};


struct aiNode;



struct aiBone {
    
    C_STRUCT aiString mName;

    
    unsigned int mNumWeights;

#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
    
    C_STRUCT aiNode *mArmature;

    
    C_STRUCT aiNode *mNode;

#endif
    
    C_STRUCT aiVertexWeight *mWeights;

    
    C_STRUCT aiMatrix4x4 mOffsetMatrix;

#ifdef __cplusplus

    
    aiBone() AI_NO_EXCEPT
            : mName(),
              mNumWeights(0),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
              mArmature(nullptr),
              mNode(nullptr),
#endif
              mWeights(nullptr),
              mOffsetMatrix() {
        
    }

    
    aiBone(const aiBone &other) :
            mName(other.mName),
            mNumWeights(other.mNumWeights),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
              mArmature(nullptr),
              mNode(nullptr),
#endif
            mWeights(nullptr),
            mOffsetMatrix(other.mOffsetMatrix) {
        copyVertexWeights(other);
    }

    void copyVertexWeights( const aiBone &other ) {
        if (other.mWeights == nullptr || other.mNumWeights == 0) {
            mWeights = nullptr;
            mNumWeights = 0;
            return;
        }

        mNumWeights = other.mNumWeights;
        if (mWeights) {
            delete[] mWeights;
        }

        mWeights = new aiVertexWeight[mNumWeights];
        ::memcpy(mWeights, other.mWeights, mNumWeights * sizeof(aiVertexWeight));
    }

    
    aiBone &operator = (const aiBone &other) {
        if (this == &other) {
            return *this;
        }

        mName = other.mName;
        mNumWeights = other.mNumWeights;
        mOffsetMatrix = other.mOffsetMatrix;
        copyVertexWeights(other);

        return *this;
    }

    
    bool operator==(const aiBone &rhs) const {
        if (mName != rhs.mName || mNumWeights != rhs.mNumWeights ) {
            return false;
        }

        for (size_t i = 0; i < mNumWeights; ++i) {
            if (mWeights[i] != rhs.mWeights[i]) {
                return false;
            }
        }

        return true;
    }
    
    ~aiBone() {
        delete[] mWeights;
    }
#endif 
};



enum aiPrimitiveType {
    
    aiPrimitiveType_POINT = 0x1,

    
    aiPrimitiveType_LINE = 0x2,

    
    aiPrimitiveType_TRIANGLE = 0x4,

    
    aiPrimitiveType_POLYGON = 0x8,

    
    aiPrimitiveType_NGONEncodingFlag = 0x10,

    
#ifndef SWIG
    _aiPrimitiveType_Force32Bit = INT_MAX
#endif
}; 


#define AI_PRIMITIVE_TYPE_FOR_N_INDICES(n) \
    ((n) > 3 ? aiPrimitiveType_POLYGON : (aiPrimitiveType)(1u << ((n)-1)))



struct aiAnimMesh {
    
    C_STRUCT aiString mName;

    
    C_STRUCT aiVector3D *mVertices;

    
    C_STRUCT aiVector3D *mNormals;

    
    C_STRUCT aiVector3D *mTangents;

    
    C_STRUCT aiVector3D *mBitangents;

    
    C_STRUCT aiColor4D *mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

    
    C_STRUCT aiVector3D *mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    
    unsigned int mNumVertices;

    
    float mWeight;

#ifdef __cplusplus
    
    aiAnimMesh() AI_NO_EXCEPT :
            mVertices(nullptr),
            mNormals(nullptr),
            mTangents(nullptr),
            mBitangents(nullptr),
            mColors {nullptr},
            mTextureCoords{nullptr},
            mNumVertices(0),
            mWeight(0.0f) {
        
    }

    
    ~aiAnimMesh() {
        delete[] mVertices;
        delete[] mNormals;
        delete[] mTangents;
        delete[] mBitangents;
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
            delete[] mTextureCoords[a];
        }
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++) {
            delete[] mColors[a];
        }
    }

    
    bool HasPositions() const {
        return mVertices != nullptr;
    }

    
    bool HasNormals() const {
        return mNormals != nullptr;
    }

    
    bool HasTangentsAndBitangents() const {
        return mTangents != nullptr;
    }

    

    bool HasVertexColors(unsigned int pIndex) const {
        return pIndex >= AI_MAX_NUMBER_OF_COLOR_SETS ? false : mColors[pIndex] != nullptr;
    }

    
    bool HasTextureCoords(unsigned int pIndex) const {
        return pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? false : mTextureCoords[pIndex] != nullptr;
    }

#endif
};



enum aiMorphingMethod {
    
    aiMorphingMethod_UNKNOWN = 0x0,

    
    aiMorphingMethod_VERTEX_BLEND = 0x1,

    
    aiMorphingMethod_MORPH_NORMALIZED = 0x2,

    
    aiMorphingMethod_MORPH_RELATIVE = 0x3,


#ifndef SWIG
    _aiMorphingMethod_Force32Bit = INT_MAX
#endif
}; 



struct aiMesh {
    
    unsigned int mPrimitiveTypes;

    
    unsigned int mNumVertices;

    
    unsigned int mNumFaces;

    
    C_STRUCT aiVector3D *mVertices;

    
    C_STRUCT aiVector3D *mNormals;

    
    C_STRUCT aiVector3D *mTangents;

    
    C_STRUCT aiVector3D *mBitangents;

    
    C_STRUCT aiColor4D *mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

    
    C_STRUCT aiVector3D *mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    
    unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    
    C_STRUCT aiFace *mFaces;

    
    unsigned int mNumBones;

    
    C_STRUCT aiBone **mBones;

    
    unsigned int mMaterialIndex;

    
    C_STRUCT aiString mName;

    
    unsigned int mNumAnimMeshes;

    
    C_STRUCT aiAnimMesh **mAnimMeshes;

    
    enum aiMorphingMethod mMethod;

    
    C_STRUCT aiAABB mAABB;

    
    C_STRUCT aiString **mTextureCoordsNames;

#ifdef __cplusplus

    
    aiMesh() AI_NO_EXCEPT
            : mPrimitiveTypes(0),
              mNumVertices(0),
              mNumFaces(0),
              mVertices(nullptr),
              mNormals(nullptr),
              mTangents(nullptr),
              mBitangents(nullptr),
              mColors{nullptr},
              mTextureCoords{nullptr},
              mNumUVComponents{0},
              mFaces(nullptr),
              mNumBones(0),
              mBones(nullptr),
              mMaterialIndex(0),
              mNumAnimMeshes(0),
              mAnimMeshes(nullptr),
              mMethod(aiMorphingMethod_UNKNOWN),
              mAABB(),
              mTextureCoordsNames(nullptr) {
        
    }

    
    ~aiMesh() {
        delete[] mVertices;
        delete[] mNormals;
        delete[] mTangents;
        delete[] mBitangents;
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
            delete[] mTextureCoords[a];
        }

        if (mTextureCoordsNames) {
            for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
                delete mTextureCoordsNames[a];
            }
            delete[] mTextureCoordsNames;
        }

        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++) {
            delete[] mColors[a];
        }

        
        if (mNumBones && mBones) {
            std::unordered_set<const aiBone *> bones;
            for (unsigned int a = 0; a < mNumBones; a++) {
                if (mBones[a]) {
                    bones.insert(mBones[a]);
                }
            }
            for (const aiBone *bone: bones) {
                delete bone;
            }
            delete[] mBones;
        }

        if (mNumAnimMeshes && mAnimMeshes) {
            for (unsigned int a = 0; a < mNumAnimMeshes; a++) {
                delete mAnimMeshes[a];
            }
            delete[] mAnimMeshes;
        }

        delete[] mFaces;
    }

    
    
    
    bool HasPositions() const {
        return mVertices != nullptr && mNumVertices > 0;
    }

    
    
    
    bool HasFaces() const {
        return mFaces != nullptr && mNumFaces > 0;
    }

    
    
    bool HasNormals() const {
        return mNormals != nullptr && mNumVertices > 0;
    }

    
    
    
    
    
    
    bool HasTangentsAndBitangents() const {
        return mTangents != nullptr && mBitangents != nullptr && mNumVertices > 0;
    }

    
    
    
    bool HasVertexColors(unsigned int index) const {
        if (index >= AI_MAX_NUMBER_OF_COLOR_SETS) {
            return false;
        }
        return mColors[index] != nullptr && mNumVertices > 0;
    }

    
    
    
    bool HasTextureCoords(unsigned int index) const {
        if (index >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return false;
        }
        return (mTextureCoords[index] != nullptr && mNumVertices > 0);
    }

    
    
    unsigned int GetNumUVChannels() const {
        unsigned int n(0);
        for (unsigned i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i++) {
            if (mTextureCoords[i]) {
                ++n;
            }
        }

        return n;
    }

    
    
    unsigned int GetNumColorChannels() const {
        unsigned int n(0);
        while (n < AI_MAX_NUMBER_OF_COLOR_SETS && mColors[n]) {
            ++n;
        }
        return n;
    }

    
    
    bool HasBones() const {
        return mBones != nullptr && mNumBones > 0;
    }

    
    
    
    bool HasTextureCoordsName(unsigned int pIndex) const {
        if (mTextureCoordsNames == nullptr || pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return false;
        }
        return mTextureCoordsNames[pIndex] != nullptr;
    }

    
    
    
    void SetTextureCoordsName(unsigned int pIndex, const aiString &texCoordsName) {
        if (pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return;
        }

        if (mTextureCoordsNames == nullptr) {
            
            mTextureCoordsNames = new aiString *[AI_MAX_NUMBER_OF_TEXTURECOORDS];
            for (size_t i=0; i<AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                mTextureCoordsNames[i] = nullptr;
            }
        }

        if (texCoordsName.length == 0) {
            delete mTextureCoordsNames[pIndex];
            mTextureCoordsNames[pIndex] = nullptr;
            return;
        }

        if (mTextureCoordsNames[pIndex] == nullptr) {
            mTextureCoordsNames[pIndex] = new aiString(texCoordsName);
            return;
        }

        *mTextureCoordsNames[pIndex] = texCoordsName;
    }

    
    
    
    const aiString *GetTextureCoordsName(unsigned int index) const {
        if (mTextureCoordsNames == nullptr || index >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return nullptr;
        }

        return mTextureCoordsNames[index];
    }

#endif 
};


struct aiSkeletonBone {
    
    int mParent;


#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
    
    
    C_STRUCT aiNode *mArmature;

    
    
    C_STRUCT aiNode *mNode;

#endif
    
    unsigned int mNumnWeights;

    
    C_STRUCT aiMesh *mMeshId;

    
    C_STRUCT aiVertexWeight *mWeights;

    
    C_STRUCT aiMatrix4x4 mOffsetMatrix;

    
    C_STRUCT aiMatrix4x4 mLocalMatrix;

#ifdef __cplusplus
    
    aiSkeletonBone() :
            mParent(-1),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
            mArmature(nullptr),
            mNode(nullptr),
#endif
            mNumnWeights(0),
            mMeshId(nullptr),
            mWeights(nullptr),
            mOffsetMatrix(),
            mLocalMatrix() {
        
    }

    
    
    aiSkeletonBone(unsigned int parent) :
            mParent(parent),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
            mArmature(nullptr),
            mNode(nullptr),
#endif
            mNumnWeights(0),
            mMeshId(nullptr),
            mWeights(nullptr),
            mOffsetMatrix(),
            mLocalMatrix() {
        
    }
    
    ~aiSkeletonBone() {
        delete[] mWeights;
        mWeights = nullptr;
    }
#endif 
};

struct aiSkeleton {
    
    C_STRUCT aiString mName;

    
    unsigned int mNumBones;

    
    C_STRUCT aiSkeletonBone **mBones;

#ifdef __cplusplus
    
    aiSkeleton() AI_NO_EXCEPT : mName(), mNumBones(0), mBones(nullptr) {
        
    }

    
    ~aiSkeleton() {
        delete[] mBones;
    }
#endif 
};
#ifdef __cplusplus
}
#endif 

#endif 

