#include <graphic/animator.h>

#include <map>
#include <iostream>
#include <assimp/Importer.hpp>

#include <graphic/bone.h>

Animator::Animator(Animation *animation)
{
    m_CurrentTime = 0.0;
    m_CurrentAnimation = animation;
    m_FinalBoneMatrices.resize(100, glm::mat4(1.0f));
    m_Speed = 1.0f;
    m_UpdateRate = 0.0f;
    m_TimeSinceLastUpdate = 0.0f;

    if(animation) {
		AddAnimation("Default", animation);
	}
}


// Helper to construct matrix from TRS
static glm::mat4 ComposeTransform(const glm::vec3& t, const glm::quat& r, const glm::vec3& s) {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), t);
    glm::mat4 rotation = glm::toMat4(r);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), s);
    return translation * rotation * scale;
}

void Animator::UpdateAnimation(float dt)
{
    if (m_CurrentAnimation)
    {
        // 1. Update Current Animation Time
        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt * m_Speed;
        m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
        if (m_CurrentTime < 0.0f) m_CurrentTime += m_CurrentAnimation->GetDuration();

        // 2. Update Next Animation Time (if exists)
        if (m_NextAnimation)
        {
            m_NextTime += m_NextAnimation->GetTicksPerSecond() * dt * m_Speed;
            m_NextTime = fmod(m_NextTime, m_NextAnimation->GetDuration());
            if (m_NextTime < 0.0f) m_NextTime += m_NextAnimation->GetDuration();
        }

        // 3. Handle CrossFading
        if (m_IsCrossFading && m_NextAnimation)
        {
            m_BlendFactor += m_TransitionSpeed * dt;
            if (m_BlendFactor >= 1.0f)
            {
                // Transition Complete: Next becomes Current
                m_CurrentAnimation = m_NextAnimation;
                m_CurrentTime = m_NextTime;
                
                m_NextAnimation = nullptr;
                m_BlendFactor = 0.0f;
                m_IsCrossFading = false;
            }
        }

        // 4. Update Rate Limiter (Optional)
        if (m_UpdateRate > 0.0f) 
        {
            m_TimeSinceLastUpdate += dt;
            float timePerFrame = 1.0f / m_UpdateRate;
            if (m_TimeSinceLastUpdate < timePerFrame) return; 
            m_TimeSinceLastUpdate = fmod(m_TimeSinceLastUpdate, timePerFrame);
        }

        // 5. Calculate Matrices
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }
}

void Animator::AddAnimation(const std::string& name, Animation* animation)
{
	if(animation) {
		m_AnimationsMap[name] = animation;
	}
}

void Animator::PlayAnimation(Animation *pAnimation)
{
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::PlayAnimation(const std::string& name)
{
	if (m_AnimationsMap.find(name) != m_AnimationsMap.end()) {
		Animation* targetAnim = m_AnimationsMap[name];
		
		if (m_CurrentAnimation != targetAnim) {
			m_CurrentAnimation = targetAnim;
			m_CurrentTime = 0.0f;
		}
	} else {
		std::cout << "[Animator] Animation not found: " << name << std::endl;
	}
}

void Animator::CrossFade(const std::string& name, float transitionDuration)
{
    if (m_AnimationsMap.find(name) == m_AnimationsMap.end()) {
        std::cout << "[Animator] CrossFade: Animation not found: " << name << std::endl;
        return;
    }

    Animation* target = m_AnimationsMap[name];
    if (target == m_CurrentAnimation && !m_IsCrossFading) return;
    if (target == m_NextAnimation && m_IsCrossFading) return; // Already fading to it

    m_NextAnimation = target;
    m_NextTime = 0.0f;
    m_BlendFactor = 0.0f;
    m_IsCrossFading = true;
    
    if (transitionDuration <= 0.0f) m_TransitionSpeed = 1000.0f; // Instant
    else m_TransitionSpeed = 1.0f / transitionDuration;
}

void Animator::PlayBlend(const std::string& nameA, const std::string& nameB, float factor)
{
    if (m_AnimationsMap.count(nameA) && m_AnimationsMap.count(nameB))
    {
        m_CurrentAnimation = m_AnimationsMap[nameA];
        m_NextAnimation = m_AnimationsMap[nameB];
        m_BlendFactor = glm::clamp(factor, 0.0f, 1.0f);
        m_IsCrossFading = false; // Manual blend, not auto transition
    }
}

void Animator::CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    // --- BLENDING LOGIC ---
    Bone *boneA = node->cachedBone; // Cached for Current Animation
    
    // Default components (Node Transform)
    // We assume node->transformation is irrelevant if bone exists? 
    // Actually standard generic node transform is used if no bone.

    if (boneA) // Bone exists in Current Animation
    {
        boneA->Update(m_CurrentTime);
        
        if (m_NextAnimation && (m_BlendFactor > 0.001f))
        {
            // Blending needed
            Bone* boneB = m_NextAnimation->FindBone(nodeName);
            
            if (boneB)
            {
                // Get TRS from A
                glm::vec3 posA = boneA->GetPosition(m_CurrentTime);
                glm::quat rotA = boneA->GetRotation(m_CurrentTime);
                glm::vec3 scaleA = boneA->GetScale(m_CurrentTime);

                // Get TRS from B
                glm::vec3 posB = boneB->GetPosition(m_NextTime);
                glm::quat rotB = boneB->GetRotation(m_NextTime);
                glm::vec3 scaleB = boneB->GetScale(m_NextTime);

                // Interpolate
                glm::vec3 pos = glm::mix(posA, posB, m_BlendFactor);
                glm::quat rot = glm::slerp(rotA, rotB, m_BlendFactor);
                glm::vec3 scale = glm::mix(scaleA, scaleB, m_BlendFactor);

                nodeTransform = ComposeTransform(pos, rot, scale);
            }
            else
            {
                // Bone missing in B, use A
                nodeTransform = boneA->GetLocalTransform();
            }
        }
        else
        {
            // No blending or Factor 0
            nodeTransform = boneA->GetLocalTransform();
        }
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    auto it = boneInfoMap.find(nodeName);

    if (it != boneInfoMap.end())
    {
        int index = it->second.id;
        glm::mat4 offset = it->second.offset;
        
        if (index < m_FinalBoneMatrices.size()) {
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}

std::vector<glm::mat4> Animator::GetFinalBoneMatrices()
{
    return m_FinalBoneMatrices;
}
