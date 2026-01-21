#include <graphic/bone.h>

#include <list>

#include <utils/assimp_glm_helpers.h>

Bone::Bone(const std::string &name, int ID, const aiNodeAnim *channel)
    : m_Name(name),
      m_ID(ID),
      m_LocalTransform(1.0f)
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

glm::vec3 Bone::GetPosition(float animationTime) { return InterpolatePosition(animationTime); }
glm::quat Bone::GetRotation(float animationTime) { return InterpolateRotation(animationTime); }
glm::vec3 Bone::GetScale(float animationTime) { return InterpolateScaling(animationTime); }

void Bone::Update(float animationTime)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), InterpolatePosition(animationTime));
    glm::mat4 rotation = glm::toMat4(glm::normalize(InterpolateRotation(animationTime)));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), InterpolateScaling(animationTime));
    m_LocalTransform = translation * rotation * scale;
}

glm::mat4 Bone::GetLocalTransform() { return m_LocalTransform; }
std::string Bone::GetBoneName() const { return m_Name; }
int Bone::GetBoneID() { return m_ID; }

int Bone::GetPositionIndex(float animationTime)
{
    if (m_LastPositionIndex >= m_NumPositions - 1 || animationTime < m_Positions[m_LastPositionIndex].timeStamp)
    {
        m_LastPositionIndex = 0;
    }

    for (int index = m_LastPositionIndex; index < m_NumPositions - 1; ++index)
    {
        if (animationTime < m_Positions[index + 1].timeStamp)
        {
            m_LastPositionIndex = index;
            return index;
        }
    }
    return 0;
}

int Bone::GetRotationIndex(float animationTime)
{
    if (m_LastRotationIndex >= m_NumRotations - 1 || animationTime < m_Rotations[m_LastRotationIndex].timeStamp)
    {
        m_LastRotationIndex = 0;
    }

    for (int index = m_LastRotationIndex; index < m_NumRotations - 1; ++index)
    {
        if (animationTime < m_Rotations[index + 1].timeStamp)
        {
            m_LastRotationIndex = index;
            return index;
        }
    }
    return 0;
}

int Bone::GetScaleIndex(float animationTime)
{
    if (m_LastScaleIndex >= m_NumScalings - 1 || animationTime < m_Scales[m_LastScaleIndex].timeStamp)
    {
        m_LastScaleIndex = 0;
    }

    for (int index = m_LastScaleIndex; index < m_NumScalings - 1; ++index)
    {
        if (animationTime < m_Scales[index + 1].timeStamp)
        {
            m_LastScaleIndex = index;
            return index;
        }
    }
    return 0;
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::vec3 Bone::InterpolatePosition(float animationTime)
{
    if (1 == m_NumPositions)
        return m_Positions[0].position;

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
                                       m_Positions[p1Index].timeStamp, animationTime);
    return glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
}

glm::quat Bone::InterpolateRotation(float animationTime)
{
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

glm::vec3 Bone::InterpolateScaling(float animationTime)
{
    if (1 == m_NumScalings)
        return m_Scales[0].scale;

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
                                       m_Scales[p1Index].timeStamp, animationTime);
    return glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
}