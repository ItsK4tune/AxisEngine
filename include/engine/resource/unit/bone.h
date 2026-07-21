#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>


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
    Bone(const std::string& name, const aiNodeAnim* channel);

    glm::mat4 GetTransform(float animationTime);
    std::string GetBoneName() const;
    int GetPositionIndex(float animationTime) const;
    int GetRotationIndex(float animationTime) const;
    int GetScaleIndex(float animationTime) const;

    glm::vec3 GetPosition(float animationTime) const;
    glm::quat GetRotation(float animationTime) const;
    glm::vec3 GetScale(float animationTime) const;

private:
    glm::vec3 InterpolatePosition(float animationTime) const;
    glm::quat InterpolateRotation(float animationTime) const;
    glm::vec3 InterpolateScaling(float animationTime) const;

    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;

    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    std::string m_Name;
};
