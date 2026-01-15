#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <assimp/scene.h>
#include <unordered_map>

#include <graphic/animation.h>

class Animator
{
public:
	Animator(Animation *animation);

	void UpdateAnimation(float dt);
	void AddAnimation(const std::string& name, Animation* animation);
	void PlayAnimation(Animation *pAnimation);
	void PlayAnimation(const std::string& name);

	void CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform);
	std::vector<glm::mat4> GetFinalBoneMatrices();

	void SetSpeed(float speed) { m_Speed = speed; }
	void SetTime(float timeInSeconds) { m_CurrentTime = timeInSeconds; }
	void SetUpdateRate(float updatesPerSecond) { m_UpdateRate = updatesPerSecond; }

	float GetDuration() const { return m_CurrentAnimation ? m_CurrentAnimation->GetDuration() : 0.0f; }

private:
	// Blending API
	void CrossFade(const std::string& name, float transitionDuration);
	void PlayBlend(const std::string& nameA, const std::string& nameB, float factor);
	
	// Helper to set blend factor directly (e.g. from Blend Tree script)
	void SetBlendFactor(float factor) { m_BlendFactor = factor; }

private:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation *m_CurrentAnimation;
	float m_CurrentTime;

	// Blending
	Animation *m_NextAnimation = nullptr;
    float m_NextTime = 0.0f;
    float m_BlendFactor = 0.0f; // 0.0 = Current, 1.0 = Next
    bool m_IsCrossFading = false;
    float m_TransitionSpeed = 0.0f;

	float m_Speed = 1.0f; 
	
	float m_UpdateRate = 0.0f;
    float m_TimeSinceLastUpdate = 0.0f;

	std::unordered_map<std::string, Animation*> m_AnimationsMap;
};
