#pragma once

#include <vector>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

class Bone
{
public:
	Bone(const std::string &name, int ID, const aiNodeAnim *channel);

	void Update(float animationTime);
	glm::mat4 GetLocalTransform();
	std::string GetBoneName() const;
	int GetBoneID();

	int GetPositionIndex(float animationTime);
	int GetRotationIndex(float animationTime);
	int GetScaleIndex(float animationTime);

	glm::vec3 GetPosition(float animationTime);
	glm::quat GetRotation(float animationTime);
	glm::vec3 GetScale(float animationTime);

private:
	glm::vec3 InterpolatePosition(float animationTime);
	glm::quat InterpolateRotation(float animationTime);
	glm::vec3 InterpolateScaling(float animationTime);

	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;
	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	int m_LastPositionIndex = 0;
	int m_LastRotationIndex = 0;
	int m_LastScaleIndex = 0;

	glm::mat4 m_LocalTransform;
	std::string m_Name;
	int m_ID;
};
