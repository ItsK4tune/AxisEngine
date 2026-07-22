#include <resource/unit/animation.h>
#include <core/logic/logger.h>
#include <render/logic/assimp_glm_helpers.h>
#include <resource/unit/model.h>
#include <render/type/shader_abi.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Animation::Animation(const std::string& animationPath, Model& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0)
    {
        LOGGER_ERROR("Animation") << "Failed to load animation or no animations found in: " << animationPath;
        m_Duration = 0.0f;
        m_TicksPerSecond = 0;
        return;
    }

    auto animation = scene->mAnimations[0];
    m_Duration = animation->mDuration;
    m_TicksPerSecond = animation->mTicksPerSecond > 0.0 ? static_cast<float>(animation->mTicksPerSecond) : 25.0f;

    ReadHierarchyData(m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, model);

    BindNodesToBones(m_RootNode);
    m_Valid = true;
}

Bone* Animation::FindBone(const std::string& name)
{
    if (m_BoneMap.find(name) != m_BoneMap.end())
    {
        return m_BoneMap[name];
    }
    return nullptr;
}

void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
{
    int size = animation->mNumChannels;

    std::unordered_map<std::string, BoneInfo>& boneInfoMap = model.GetBoneInfoMap();
    int& boneCount = model.GetBoneCount();

    m_Bones.reserve(size);

    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            if (boneCount < ShaderABI::MaxBones)
            {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            else
            {
                LOGGER_WARN("Animation") << "Max bone limit (" << ShaderABI::MaxBones
                                         << ") exceeded while processing missing bone: " << boneName;
                continue;
            }
        }

        m_Bones.emplace_back(channel->mNodeName.data, channel);
    }

    m_BoneInfoMap = boneInfoMap;

    m_BoneMap.clear();
    for (auto& bone : m_Bones)
    {
        m_BoneMap[bone.GetBoneName()] = &bone;
    }
}

void Animation::ReadHierarchyData(BoneNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++)
    {
        BoneNodeData newData;
        ReadHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

void Animation::BindNodesToBones(BoneNodeData& node)
{
    auto iter = m_BoneMap.find(node.name);
    if (iter != m_BoneMap.end())
    {
        node.cachedBone = iter->second;
    }

    for (auto& child : node.children)
    {
        BindNodesToBones(child);
    }
}
