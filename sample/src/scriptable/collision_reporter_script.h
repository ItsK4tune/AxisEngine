#pragma once
#include <axis_all.h>

class CollisionReporterScript : public Scriptable
{
public:
    inline static int s_CollisionEnterCount = 0;
    inline static int s_TriggerEnterCount = 0;

    void OnCollisionEnter(entt::entity other) override
    {
        (void)other;
        ++s_CollisionEnterCount;
        Mark(glm::vec4(1.0f, 0.55f, 0.12f, 1.0f), glm::vec3(0.8f, 0.25f, 0.05f));
    }

    void OnTriggerEnter(entt::entity other) override
    {
        (void)other;
        ++s_TriggerEnterCount;
        Mark(glm::vec4(0.2f, 0.95f, 1.0f, 1.0f), glm::vec3(0.0f, 0.7f, 0.9f));
    }

private:
    void Mark(const glm::vec4& color, const glm::vec3& emission)
    {
        if (HasComponent<MeshRendererComponent>())
            GetComponent<MeshRendererComponent>().color = color;
        if (HasComponent<MaterialComponent>())
        {
            auto& mat = GetComponent<MaterialComponent>();
            mat.desc.emission = emission;
            mat.gpu.dirty = true;
        }
    }
};
