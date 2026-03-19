#define GLM_ENABLE_EXPERIMENTAL
#include <render/unit/bone.h>
#include <list>
#include <render/logic/assimp_glm_helpers.h>

Bone::Bone(const std::string &name, int ID, const aiNodeAnim *channel)
    : m_Name(name),
      m_ID(ID)
{
    m_NumPositions = channel->mNumPositionKeys;

    for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        float timeStamp = channel->mPositionKeys[positionIndex].mTime;
        KeyPosition data;
        data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
        data.timeStamp = timeStamp;
        m_Positions.push_back(data);
    }

    m_NumRotations = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
        KeyRotation data;
        data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
        data.timeStamp = timeStamp;
        m_Rotations.push_back(data);
    }

    m_NumScalings = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
    {
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        float timeStamp = channel->mScalingKeys[keyIndex].mTime;
        KeyScale data;
        data.scale = AssimpGLMHelpers::GetGLMVec(scale);
        data.timeStamp = timeStamp;
        m_Scales.push_back(data);
    }
}

glm::vec3 Bone::GetPosition(float animationTime) const { return InterpolatePosition(animationTime); }
glm::quat Bone::GetRotation(float animationTime) const { return InterpolateRotation(animationTime); }
glm::vec3 Bone::GetScale(float animationTime) const { return InterpolateScaling(animationTime); }

glm::mat4 Bone::GetTransform(float animationTime)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), InterpolatePosition(animationTime));
    glm::mat4 rotation = glm::toMat4(glm::normalize(InterpolateRotation(animationTime)));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), InterpolateScaling(animationTime));
    return translation * rotation * scale;
}

std::string Bone::GetBoneName() const { return m_Name; }
int Bone::GetBoneID() const { return m_ID; }

int Bone::GetPositionIndex(float animationTime) const
{
    for (int index = 0; index < m_NumPositions - 1; ++index)
    {
        if (animationTime < m_Positions[index + 1].timeStamp)
            return index;
    }
    return 0;
}

int Bone::GetRotationIndex(float animationTime) const
{
    for (int index = 0; index < m_NumRotations - 1; ++index)
    {
        if (animationTime < m_Rotations[index + 1].timeStamp)
            return index;
    }
    return 0;
}

int Bone::GetScaleIndex(float animationTime) const
{
    for (int index = 0; index < m_NumScalings - 1; ++index)
    {
        if (animationTime < m_Scales[index + 1].timeStamp)
            return index;
    }
    return 0;
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const
{
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0f)
        return 0.0f;
    return (animationTime - lastTimeStamp) / framesDiff;
}

glm::vec3 Bone::InterpolatePosition(float animationTime) const
{
    if (m_NumPositions == 0)
        return glm::vec3(0.0f);
    if (1 == m_NumPositions)
        return m_Positions[0].position;

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
                                       m_Positions[p1Index].timeStamp, animationTime);
    return glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
}

glm::quat Bone::InterpolateRotation(float animationTime) const
{
    if (m_NumRotations == 0)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (1 == m_NumRotations)
    {
        return glm::normalize(m_Rotations[0].orientation);
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
                                       m_Rotations[p1Index].timeStamp, animationTime);
    return glm::normalize(glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor));
}

glm::vec3 Bone::InterpolateScaling(float animationTime) const
{
    if (m_NumScalings == 0)
        return glm::vec3(1.0f);
    if (1 == m_NumScalings)
        return m_Scales[0].scale;

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
                                       m_Scales[p1Index].timeStamp, animationTime);
    return glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
}
