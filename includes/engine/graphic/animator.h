#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <assimp/scene.h>
#include <unordered_map>

#include <engine/graphic/animation.h>

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
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation *m_CurrentAnimation;
	float m_CurrentTime;

	float m_Speed = 1.0f; 
	
	float m_UpdateRate = 0.0f;
    float m_TimeSinceLastUpdate = 0.0f;

	std::unordered_map<std::string, Animation*> m_AnimationsMap;
};
