#pragma once

#include <assimp/scene.h>
#include <render/type/graphics_types.h>
#include <resource/unit/bone.h>
#include <resource/unit/bone_info.h>
#include <resource/unit/model.h>
#include <glm/glm.hpp>
#include <map>
#include <unordered_map>
#include <vector>


class Animation
{
public:
    Animation() = default;
    Animation(const std::string& animationPath, Model& model);
    ~Animation() = default;

    Bone* FindBone(const std::string& name);

    bool IsValid() const
    {
        return m_Valid;
    }

    inline float GetTicksPerSecond()
    {
        return m_TicksPerSecond;
    }
    inline float GetDuration()
    {
        return m_Duration;
    }
    inline BoneNodeData& GetRootNode()
    {
        return m_RootNode;
    }
    inline const std::unordered_map<std::string, BoneInfo>& GetBoneIDMap()
    {
        return m_BoneInfoMap;
    }

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;
    bool m_Valid = false;
    std::vector<Bone> m_Bones;
    std::unordered_map<std::string, Bone*> m_BoneMap;

    BoneNodeData m_RootNode;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;

    void ReadMissingBones(const aiAnimation* animation, Model& model);
    void ReadHierarchyData(BoneNodeData& dest, const aiNode* src);

    void BindNodesToBones(BoneNodeData& node);
};
