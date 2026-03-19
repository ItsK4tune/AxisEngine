


#pragma once
#ifndef AI_SCENE_COMBINER_H_INC
#define AI_SCENE_COMBINER_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/ai_assert.h>
#include <assimp/types.h>

#include <cstddef>
#include <cstdint>
#include <list>
#include <set>
#include <vector>

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiTexture;
struct aiCamera;
struct aiLight;
struct aiMetadata;
struct aiBone;
struct aiMesh;
struct aiAnimMesh;
struct aiAnimation;
struct aiNodeAnim;
struct aiMeshMorphAnim;

namespace Assimp {



struct AttachmentInfo {
    AttachmentInfo() :
            scene(nullptr),
            attachToNode(nullptr) {}

    AttachmentInfo(aiScene *_scene, aiNode *_attachToNode) :
            scene(_scene), attachToNode(_attachToNode) {}

    aiScene *scene;
    aiNode *attachToNode;
};


struct NodeAttachmentInfo {
    NodeAttachmentInfo() :
            node(nullptr),
            attachToNode(nullptr),
            resolved(false),
            src_idx(SIZE_MAX) {}

    NodeAttachmentInfo(aiNode *_scene, aiNode *_attachToNode, size_t idx) :
            node(_scene), attachToNode(_attachToNode), resolved(false), src_idx(idx) {}

    aiNode *node;
    aiNode *attachToNode;
    bool resolved;
    size_t src_idx;
};



#define AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES 0x1


#define AI_INT_MERGE_SCENE_GEN_UNIQUE_MATNAMES 0x2


#define AI_INT_MERGE_SCENE_DUPLICATES_DEEP_CPY 0x4


#define AI_INT_MERGE_SCENE_RESOLVE_CROSS_ATTACHMENTS 0x8


#define AI_INT_MERGE_SCENE_GEN_UNIQUE_NAMES_IF_NECESSARY 0x10

typedef std::pair<aiBone *, unsigned int> BoneSrcIndex;



struct BoneWithHash : public std::pair<uint32_t, aiString *> {
    std::vector<BoneSrcIndex> pSrcBones;
};



struct SceneHelper {
    SceneHelper() :
            scene(nullptr),
            idlen(0) {
        id[0] = 0;
    }

    explicit SceneHelper(aiScene *_scene) :
            scene(_scene), idlen(0) {
        id[0] = 0;
    }

    AI_FORCE_INLINE aiScene *operator->() const {
        return scene;
    }

    
    aiScene *scene;

    
    char id[32];

    
    unsigned int idlen;

    
    std::set<unsigned int> hashes;
};



class ASSIMP_API SceneCombiner {
public:
    
    SceneCombiner() = delete;
    ~SceneCombiner() = delete;

    
    
    static void MergeScenes(aiScene **dest, std::vector<aiScene *> &src,
            unsigned int flags = 0);

    
    
    static void MergeScenes(aiScene **dest, aiScene *master,
            std::vector<AttachmentInfo> &src,
            unsigned int flags = 0);

    
    
    static void MergeMeshes(aiMesh **dest, unsigned int flags,
            std::vector<aiMesh *>::const_iterator begin,
            std::vector<aiMesh *>::const_iterator end);

    
    
    static void MergeBones(aiMesh *out, std::vector<aiMesh *>::const_iterator it,
            std::vector<aiMesh *>::const_iterator end);

    
    
    static void MergeMaterials(aiMaterial **dest,
            std::vector<aiMaterial *>::const_iterator begin,
            std::vector<aiMaterial *>::const_iterator end);

    
    
    static void BuildUniqueBoneList(std::list<BoneWithHash> &asBones,
            std::vector<aiMesh *>::const_iterator it,
            std::vector<aiMesh *>::const_iterator end);

    
    
    static void AddNodePrefixes(aiNode *node, const char *prefix,
            unsigned int len);

    
    
    static void OffsetNodeMeshIndices(aiNode *node, unsigned int offset);

    
    
    static void AttachToGraph(aiScene *master,
            std::vector<NodeAttachmentInfo> &srcList);

    static void AttachToGraph(aiNode *attach,
            std::vector<NodeAttachmentInfo> &srcList);

    
    
    static void CopyScene(aiScene **dest, const aiScene *source, bool allocate = true);

    
    
    static void CopySceneFlat(aiScene **dest, const aiScene *source);

    
    
    static void Copy(aiMesh **dest, const aiMesh *src);

    
    static void Copy(aiAnimMesh **dest, const aiAnimMesh *src);
    static void Copy(aiMaterial **dest, const aiMaterial *src);
    static void Copy(aiTexture **dest, const aiTexture *src);
    static void Copy(aiAnimation **dest, const aiAnimation *src);
    static void Copy(aiCamera **dest, const aiCamera *src);
    static void Copy(aiBone **dest, const aiBone *src);
    static void Copy(aiLight **dest, const aiLight *src);
    static void Copy(aiNodeAnim **dest, const aiNodeAnim *src);
    static void Copy(aiMeshMorphAnim **dest, const aiMeshMorphAnim *src);
    static void Copy(aiMetadata **dest, const aiMetadata *src);
    static void Copy(aiString **dest, const aiString *src);

    
    static void Copy(aiNode **dest, const aiNode *src);

private:
    
    
    static void AddNodePrefixesChecked(aiNode *node, const char *prefix,
            unsigned int len,
            std::vector<SceneHelper> &input,
            unsigned int cur);

    
    
    static void AddNodeHashes(aiNode *node, std::set<unsigned int> &hashes);

    
    
    static bool FindNameMatch(const aiString &name,
            std::vector<SceneHelper> &input, unsigned int cur);
};

} 

#endif 
