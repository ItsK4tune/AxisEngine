#include <graphic/geometry/animator.h>

#include <map>
#include <iostream>
#include <assimp/Importer.hpp>

#include <graphic/geometry/bone.h>
#include <utils/logger.h>

Animator::Animator(std::shared_ptr<Animation> animation)
{
    m_CurrentTime = 0.0;
    m_CurrentAnimation = animation;
    m_FinalBoneMatrices.resize(200, glm::mat4(1.0f));
    m_Speed = 1.0f;
    m_UpdateRate = 0.0f;
    m_TimeSinceLastUpdate = 0.0f;

    if (animation)
    {
        AddAnimation("Default", animation);
    }
}

static glm::mat4 ComposeTransform(const glm::vec3 &t, const glm::quat &r, const glm::vec3 &s)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), t);
    glm::mat4 rotation = glm::toMat4(r);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), s);
    return translation * rotation * scale;
}

void Animator::UpdateAnimation(float dt)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_CurrentAnimation)
    {
        float duration = m_CurrentAnimation->GetDuration();
        if (duration <= 0.0f)
            return;

        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt * m_Speed;
        m_CurrentTime = fmod(m_CurrentTime, duration);
        if (m_CurrentTime < 0.0f)
            m_CurrentTime += duration;

        if (m_NextAnimation)
        {
            float nextDuration = m_NextAnimation->GetDuration();
            if (nextDuration > 0.0f)
            {
                m_NextTime += m_NextAnimation->GetTicksPerSecond() * dt * m_Speed;
                m_NextTime = fmod(m_NextTime, nextDuration);
                if (m_NextTime < 0.0f)
                    m_NextTime += nextDuration;
            }
        }

        if (m_IsCrossFading && m_NextAnimation)
        {
            m_BlendFactor += m_TransitionSpeed * dt;
            if (m_BlendFactor >= 1.0f)
            {
                m_CurrentAnimation = m_NextAnimation;
                m_CurrentTime = m_NextTime;

                m_NextAnimation = nullptr;
                m_BlendFactor = 0.0f;
                m_IsCrossFading = false;
            }
        }

        if (m_UpdateRate > 0.0f)
        {
            m_TimeSinceLastUpdate += dt;
            float timePerFrame = 1.0f / m_UpdateRate;
            if (m_TimeSinceLastUpdate < timePerFrame)
                return;
            m_TimeSinceLastUpdate = fmod(m_TimeSinceLastUpdate, timePerFrame);
        }

        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), 0);
    }
}

void Animator::AddAnimation(const std::string &name, std::shared_ptr<Animation> animation)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (animation)
    {
        m_AnimationsMap[name] = animation;
    }
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
    if (!pAnimation) return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::PlayAnimation(const std::string &name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_AnimationsMap.find(name) != m_AnimationsMap.end())
    {
        std::shared_ptr<Animation> targetAnim = m_AnimationsMap[name];

        if (m_CurrentAnimation != targetAnim)
        {
            m_CurrentAnimation = targetAnim;
            m_CurrentTime = 0.0f;
        }
    }
    else
    {
        LOGGER_ERROR("Animator") << "Animation not found: " << name;
    }
}

void Animator::CrossFade(const std::string &name, float transitionDuration)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_AnimationsMap.find(name) == m_AnimationsMap.end())
    {
        LOGGER_ERROR("Animator") << "CrossFade: Animation not found: " << name;
        return;
    }

    std::shared_ptr<Animation> target = m_AnimationsMap[name];
    if (target == m_CurrentAnimation && !m_IsCrossFading)
        return;
    if (target == m_NextAnimation && m_IsCrossFading)
        return;

    m_NextAnimation = target;
    m_NextTime = 0.0f;
    m_BlendFactor = 0.0f;
    m_IsCrossFading = true;

    if (transitionDuration <= 0.0f)
        m_TransitionSpeed = 1000.0f;
    else
        m_TransitionSpeed = 1.0f / transitionDuration;
}

void Animator::PlayBlend(const std::string &nameA, const std::string &nameB, float factor)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_AnimationsMap.count(nameA) && m_AnimationsMap.count(nameB))
    {
        m_CurrentAnimation = m_AnimationsMap[nameA];
        m_NextAnimation = m_AnimationsMap[nameB];
        m_BlendFactor = glm::clamp(factor, 0.0f, 1.0f);
        m_IsCrossFading = false;
    }
}

void Animator::CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform, int depth)
{
    if (depth > 100)
    {
        LOGGER_ERROR("Animator") << "Recursion depth too high in CalculateBoneTransform! Potential cycle.";
        return;
    }
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone *boneA = node->cachedBone;

    if (boneA)
    {
        if (m_NextAnimation && (m_BlendFactor > 0.001f))
        {
            Bone *boneB = m_NextAnimation->FindBone(nodeName);

            if (boneB)
            {
                glm::vec3 posA = boneA->GetPosition(m_CurrentTime);
                glm::quat rotA = boneA->GetRotation(m_CurrentTime);
                glm::vec3 scaleA = boneA->GetScale(m_CurrentTime);

                glm::vec3 posB = boneB->GetPosition(m_NextTime);
                glm::quat rotB = boneB->GetRotation(m_NextTime);
                glm::vec3 scaleB = boneB->GetScale(m_NextTime);

                glm::vec3 pos = glm::mix(posA, posB, m_BlendFactor);
                glm::quat rot = glm::slerp(rotA, rotB, m_BlendFactor);
                glm::vec3 scale = glm::mix(scaleA, scaleB, m_BlendFactor);

                nodeTransform = ComposeTransform(pos, rot, scale);
            }
            else
            {
                nodeTransform = boneA->GetTransform(m_CurrentTime);
            }
        }
        else
        {
            nodeTransform = boneA->GetTransform(m_CurrentTime);
        }
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    const auto &boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    auto it = boneInfoMap.find(nodeName);

    if (it != boneInfoMap.end())
    {
        int index = it->second.id;
        glm::mat4 offset = it->second.offset;

        if (index < m_FinalBoneMatrices.size())
        {
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation, depth + 1);
}

std::vector<glm::mat4> Animator::GetFinalBoneMatrices()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_FinalBoneMatrices;
}

void Animator::SetIdentityMatrices(int boneCount)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_FinalBoneMatrices.size() < (size_t)boneCount)
        m_FinalBoneMatrices.resize(boneCount, glm::mat4(1.0f));

    for (int i = 0; i < boneCount; ++i)
        m_FinalBoneMatrices[i] = glm::mat4(1.0f);
}
