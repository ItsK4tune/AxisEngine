#pragma once

#include <assimp/scene.h>
#include <resource/unit/animation.h>
#include <glm/glm.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>


class Animator
{
public:
    Animator(std::shared_ptr<Animation> animation);
    ~Animator() = default;

    void UpdateAnimation(float dt, const glm::mat4& rootTransform = glm::mat4(1.0f));
    void AddAnimation(const std::string& name, std::shared_ptr<Animation> animation);
    void PlayAnimation(std::shared_ptr<Animation> pAnimation);
    void PlayAnimation(const std::string& name);
    bool HasAnimation(const std::string& name) const;

    void CalculateBoneTransform(const BoneNodeData* node, glm::mat4 parentTransform, int depth);
    const std::vector<glm::mat4>& GetFinalBoneMatrices();

    void SetSpeed(float speed)
    {
        m_Speed = speed;
    }
    void SetTime(float timeInSeconds)
    {
        m_CurrentTime = timeInSeconds;
    }
    float GetCurrentTime() const
    {
        return m_CurrentTime;
    }
    void SetUpdateRate(float updatesPerSecond)
    {
        m_UpdateRate = updatesPerSecond;
    }

    float GetDuration() const
    {
        return m_CurrentAnimation ? m_CurrentAnimation->GetDuration() : 0.0f;
    }
    float GetNormalizedTime() const
    {
        const float duration = GetDuration();
        return duration > 0.0f ? m_CurrentTime / duration : 0.0f;
    }
    bool IsCrossFading() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_IsCrossFading;
    }

    void CrossFade(const std::string& name, float transitionDuration);
    void PlayBlend(const std::string& nameA, const std::string& nameB, float factor);
    void SetBlendFactor(float factor)
    {
        m_BlendFactor = factor;
    }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    std::shared_ptr<Animation> m_CurrentAnimation;
    float m_CurrentTime;

    std::shared_ptr<Animation> m_NextAnimation = nullptr;
    float m_NextTime = 0.0f;
    float m_BlendFactor = 0.0f;
    bool m_IsCrossFading = false;
    float m_TransitionSpeed = 0.0f;

    float m_Speed = 1.0f;

    float m_UpdateRate = 0.0f;
    float m_TimeSinceLastUpdate = 0.0f;

    std::unordered_map<std::string, std::shared_ptr<Animation>> m_AnimationsMap;
    mutable std::mutex m_Mutex;
};
