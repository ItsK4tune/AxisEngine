#include <engine/graphic/animator.h>

#include <map>

#include <assimp/Importer.hpp>

#include <engine/graphic/bone.h>

Animator::Animator(Animation *animation)
{
    m_CurrentTime = 0.0;
    m_CurrentAnimation = animation;
    m_FinalBoneMatrices.resize(100, glm::mat4(1.0f));
    m_Speed = 1.0f;
    m_UpdateRate = 0.0f;
    m_TimeSinceLastUpdate = 0.0f;
}

void Animator::UpdateAnimation(float dt)
{
    if (m_CurrentAnimation)
    {
        float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();
        float duration = m_CurrentAnimation->GetDuration();

        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt * m_Speed;

        if (m_CurrentTime >= duration)
        {
            m_CurrentTime = fmod(m_CurrentTime, duration);
        }
        else if (m_CurrentTime < 0.0f)
        {
            m_CurrentTime = duration + fmod(m_CurrentTime, duration);
        }

        if (m_UpdateRate > 0.0f) 
        {
            m_TimeSinceLastUpdate += dt;
            float timePerFrame = 1.0f / m_UpdateRate;

            if (m_TimeSinceLastUpdate < timePerFrame) {
                return; 
            }

            m_TimeSinceLastUpdate = fmod(m_TimeSinceLastUpdate, timePerFrame);
        }


        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }
}

void Animator::PlayAnimation(Animation *pAnimation)
{
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone *Bone = node->cachedBone;

    if (Bone)
    {
        Bone->Update(m_CurrentTime);
        nodeTransform = Bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        glm::mat4 offset = boneInfoMap[nodeName].offset;
        m_FinalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}

std::vector<glm::mat4> Animator::GetFinalBoneMatrices()
{
    return m_FinalBoneMatrices;
}
