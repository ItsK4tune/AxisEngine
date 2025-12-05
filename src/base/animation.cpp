#include <engine/animation.h>
#include <engine/model.h>
#include <engine/assimp_glm_helpers.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Animation::Animation() = default;

Animation::Animation(const std::string &animationPath, Model *model)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);
    auto animation = scene->mAnimations[0];
    m_Duration = animation->mDuration;
    m_TicksPerSecond = animation->mTicksPerSecond;
    aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
    globalTransformation = globalTransformation.Inverse();
    ReadHierarchyData(m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, *model);
}

Animation::~Animation()
{
}

Bone *Animation::FindBone(const std::string &name)
{
    auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
                             [&](const Bone &Bone)
                             {
                                 return Bone.GetBoneName() == name;
                             });
    if (iter == m_Bones.end())
        return nullptr;
    else
        return &(*iter);
}

void Animation::ReadMissingBones(const aiAnimation *animation, Model &model)
{
    int size = animation->mNumChannels;

    std::map<std::string, BoneInfo> &boneInfoMap = model.GetBoneInfoMap(); 
    int &boneCount = model.GetBoneCount();      

    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }
        m_Bones.push_back(Bone(channel->mNodeName.data,
                               boneInfoMap[channel->mNodeName.data].id, channel));
    }

    m_BoneInfoMap = boneInfoMap;
}

void Animation::ReadHierarchyData(AssimpNodeData &dest, const aiNode *src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ReadHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}
