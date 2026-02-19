


#pragma once
#ifndef AI_SCENE_H_INC
#define AI_SCENE_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>
#include <assimp/texture.h>
#include <assimp/mesh.h>
#include <assimp/light.h>
#include <assimp/camera.h>
#include <assimp/material.h>
#include <assimp/anim.h>
#include <assimp/metadata.h>

#ifdef __cplusplus
#  include <cstdlib>
extern "C" {
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif




struct ASSIMP_API aiNode {
    
    C_STRUCT aiString mName;

    
    C_STRUCT aiMatrix4x4 mTransformation;

    
    C_STRUCT aiNode* mParent;

    
    unsigned int mNumChildren;

    
    C_STRUCT aiNode** mChildren;

    
    unsigned int mNumMeshes;

    
    unsigned int* mMeshes;

    
    C_STRUCT aiMetadata* mMetaData;

#ifdef __cplusplus
    
    aiNode();

    
    explicit aiNode(const std::string& name);

    
    ~aiNode();

    
    inline const aiNode* FindNode(const aiString& name) const {
        return FindNode(name.data);
    }

    inline aiNode* FindNode(const aiString& name) {
        return FindNode(name.data);
    }

    
    const aiNode* FindNode(const char* name) const;
    aiNode* FindNode(const char* name);

    
    
    const aiNode *findBoneNode(const aiBone *bone) const {
        if (bone == nullptr) {
            return nullptr;
        }

        if (mName == bone->mName) {
            return this;
        }

        for (unsigned int i = 0; i < mNumChildren; ++i) {
            aiNode *aChild = mChildren[i];
            if (aChild == nullptr) {
                continue;
            }

            const aiNode *foundFromChild = nullptr;
            foundFromChild = aChild->findBoneNode(bone);
            if (foundFromChild) {
                return foundFromChild;
            }
        }

        return nullptr;
    }

    
    void addChildren(unsigned int numChildren, aiNode **children);
#endif 
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif



#define AI_SCENE_FLAGS_INCOMPLETE   0x1


#define AI_SCENE_FLAGS_VALIDATED    0x2


#define AI_SCENE_FLAGS_VALIDATION_WARNING   0x4


#define AI_SCENE_FLAGS_NON_VERBOSE_FORMAT   0x8

 
#define AI_SCENE_FLAGS_TERRAIN 0x10

 
#define AI_SCENE_FLAGS_ALLOW_SHARED			0x20




struct ASSIMP_API aiScene {
    
    unsigned int mFlags;

    
    C_STRUCT aiNode* mRootNode;

    
    unsigned int mNumMeshes;

    
    C_STRUCT aiMesh** mMeshes;

    
    unsigned int mNumMaterials;

    
    C_STRUCT aiMaterial** mMaterials;

    
    unsigned int mNumAnimations;

    
    C_STRUCT aiAnimation** mAnimations;

    
    unsigned int mNumTextures;

    
    C_STRUCT aiTexture** mTextures;

    
    unsigned int mNumLights;

    
    C_STRUCT aiLight** mLights;

    
    unsigned int mNumCameras;

    
    C_STRUCT aiCamera** mCameras;

    
    C_STRUCT aiMetadata* mMetaData;

    
    C_STRUCT aiString mName;

    
    unsigned int mNumSkeletons;

    
    C_STRUCT aiSkeleton **mSkeletons;

#ifdef __cplusplus

    
    aiScene();

    
    ~aiScene();

    
    
    inline bool HasMeshes() const {
        return mMeshes != nullptr && mNumMeshes > 0;
    }

    
    
    inline bool HasMaterials() const {
        return mMaterials != nullptr && mNumMaterials > 0;
    }

    
    inline bool HasLights() const {
        return mLights != nullptr && mNumLights > 0;
    }

    
    inline bool HasTextures() const {
        return mTextures != nullptr && mNumTextures > 0;
    }

    
    inline bool HasCameras() const {
        return mCameras != nullptr && mNumCameras > 0;
    }

    
    inline bool HasAnimations() const {
        return mAnimations != nullptr && mNumAnimations > 0;
    }

    
    inline bool HasSkeletons() const {
        return mSkeletons != nullptr && mNumSkeletons > 0;
    }

    
    static const char* GetShortFilename(const char* filename) {
        const char* lastSlash = strrchr(filename, '/');
        const char* lastBackSlash = strrchr(filename, '\\');
        if (lastSlash < lastBackSlash) {
            lastSlash = lastBackSlash;
        }
        const char* shortFilename = lastSlash != nullptr ? lastSlash + 1 : filename;
        return shortFilename;
    }

    
    const aiTexture* GetEmbeddedTexture(const char* filename) const {
        return GetEmbeddedTextureAndIndex(filename).first;
    }

    
    std::pair<const aiTexture*, int> GetEmbeddedTextureAndIndex(const char* filename) const {
        if (nullptr==filename) {
            return std::make_pair(nullptr, -1);
        }
        
        if ('*' == *filename) {
            int index = std::atoi(filename + 1);
            if (0 > index || mNumTextures <= static_cast<unsigned>(index)) {
                return std::make_pair(nullptr, -1);
            }
            return std::make_pair(mTextures[index], index);
        }
        
        const char* shortFilename = GetShortFilename(filename);
        if (nullptr == shortFilename) {
            return std::make_pair(nullptr, -1);
        }

        for (unsigned int i = 0; i < mNumTextures; i++) {
            const char* shortTextureFilename = GetShortFilename(mTextures[i]->mFilename.C_Str());
            if (strcmp(shortTextureFilename, shortFilename) == 0) {
                return std::make_pair(mTextures[i], static_cast<int>(i));
            }
        }
        return std::make_pair(nullptr, -1);
    }

    
    inline aiBone *findBone(const aiString &name) const {
        for (size_t m = 0; m < mNumMeshes; m++) {
            aiMesh *mesh = mMeshes[m];
            if (mesh == nullptr) {
                continue;
            }

            for (size_t b = 0; b < mesh->mNumBones; b++) {
                aiBone *bone = mesh->mBones[b];
                if (bone == nullptr) {
                    continue;
                }
                if (name == bone->mName) {
                    return bone;
                }
            }
        }
        return nullptr;
    }

#endif 

    
#ifdef __cplusplus
    void* mPrivate;
#else
    char* mPrivate;
#endif

};

#ifdef __cplusplus
}
#endif 

#endif 
