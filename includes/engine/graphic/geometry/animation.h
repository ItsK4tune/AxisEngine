#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <graphic/geometry/bone.h>
#include <graphic/geometry/animdata.h>
#include <graphic/geometry/model.h>

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;

	Bone *cachedBone = nullptr;
};

class Animation
{
public:
	Animation();
	Animation(const std::string &animationPath, Model *model);
	~Animation();

	Bone *FindBone(const std::string &name);

	inline float GetTicksPerSecond() { return m_TicksPerSecond; }
	inline float GetDuration() { return m_Duration; }
	inline AssimpNodeData &GetRootNode() { return m_RootNode; }
	inline const std::unordered_map<std::string, BoneInfo> &GetBoneIDMap() { return m_BoneInfoMap; }

private:
	float m_Duration;
	int m_TicksPerSecond;
	std::vector<Bone> m_Bones;
	std::unordered_map<std::string, Bone *> m_BoneMap;

	AssimpNodeData m_RootNode;
	std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;

	void ReadMissingBones(const aiAnimation *animation, Model &model);
	void ReadHierarchyData(AssimpNodeData &dest, const aiNode *src);

	void BindNodesToBones(AssimpNodeData& node); 
};
