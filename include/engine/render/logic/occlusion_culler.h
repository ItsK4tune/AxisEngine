#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <scene/logic/scene.h>
#include <vector>

class IGraphicsContext;
class Shader;

class OcclusionCuller {
public:
    void Initialize(IGraphicsContext& context, std::shared_ptr<Shader> shader);
    void Shutdown();

    void UpdateResults(Scene& scene);
    void RenderQueries(Scene& scene, const glm::mat4& proj, const glm::mat4& view, float alpha);

    void AddQuery(uint32_t id) { m_OcclusionQueries.push_back(id); }
private:
    void InitOcclusionCube();

    IGraphicsContext* m_Context = nullptr;
    std::shared_ptr<Shader> m_Shader;
    
    unsigned int m_CubeVAO = 0;
    unsigned int m_CubeVBO = 0;
    unsigned int m_CubeEBO = 0;
    std::vector<uint32_t> m_OcclusionQueries;
};