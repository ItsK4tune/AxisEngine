#include <rendering/renderer/occlusion_culler.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <rendering/interfaces/i_buffer_manager.h>
#include <rendering/interfaces/i_query_manager.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <rendering/interfaces/i_draw_context.h>
#include <rendering/core/shader.h>
#include <ecs/components/occlusion_component.h>
#include <ecs/component.h>

void OcclusionCuller::Init(IGraphicsContext& context, std::shared_ptr<Shader> shader) {
    m_Context = &context;
    m_Shader = shader;
    InitOcclusionCube();
}

void OcclusionCuller::Shutdown() {
    if (!m_Context) return;
    auto& bm = m_Context->GetBufferManager();
    auto& qm = m_Context->GetQueryManager();

    if (m_CubeVAO != 0) bm.DeleteVertexArray(m_CubeVAO);
    if (m_CubeVBO != 0) bm.DeleteBuffer(m_CubeVBO);
    if (m_CubeEBO != 0) bm.DeleteBuffer(m_CubeEBO);

    for (uint32_t queryId : m_OcclusionQueries) {
        qm.DeleteQuery(queryId);
    }
    m_OcclusionQueries.clear();
}

void OcclusionCuller::InitOcclusionCube() {
    float vertices[] = {
        -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f
    };
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,  4, 5, 6, 6, 7, 4,  0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,  0, 1, 5, 5, 4, 0,  3, 2, 6, 6, 7, 3
    };

    auto& bm = m_Context->GetBufferManager();
    m_CubeVAO = bm.GenVertexArray();
    m_CubeVBO = bm.GenBuffer();
    m_CubeEBO = bm.GenBuffer();

    bm.BindVertexArray(m_CubeVAO);
    bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_CubeVBO);
    bm.BufferData(Graphics::BufferType::ArrayBuffer, sizeof(vertices), vertices, Graphics::BufferUsage::StaticDraw);
    bm.BindBuffer(Graphics::BufferType::ElementArrayBuffer, m_CubeEBO);
    bm.BufferData(Graphics::BufferType::ElementArrayBuffer, sizeof(indices), indices, Graphics::BufferUsage::StaticDraw);
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, Graphics::DataType::Float, false, 3 * sizeof(float), (void*)0);
    bm.BindVertexArray(0);
}

void OcclusionCuller::UpdateResults(Scene& scene) {
    if (!m_Context) return;
    auto& qm = m_Context->GetQueryManager();
    auto view = scene.registry.view<OcclusionComponent>();

    for (auto entity : view) {
        auto& occ = view.get<OcclusionComponent>(entity);
        if (occ.queryPending && occ.lastQueryId != 0) {
            if (qm.IsResultAvailable(occ.lastQueryId)) {
                uint32_t samples = qm.GetQueryResult(occ.lastQueryId);
                occ.isVisible = (samples > 0);
                occ.queryPending = false;
            }
        }
    }
}

void OcclusionCuller::RenderQueries(Scene& scene, const glm::mat4& proj, const glm::mat4& view, float alpha) {
    if (!m_Shader || m_CubeVAO == 0 || !m_Context) return;

    auto& rsm = m_Context->GetRenderStateManager();
    auto& qm = m_Context->GetQueryManager();
    auto& bm = m_Context->GetBufferManager();

    rsm.ColorMask(false, false, false, false);
    rsm.DepthMask(false);

    m_Shader->use();
    m_Shader->setMat4("projection", proj);
    m_Shader->setMat4("view", view);

    bm.BindVertexArray(m_CubeVAO);

    auto occView = scene.registry.view<WorldTransformComponent, MeshRendererComponent, OcclusionComponent>();

    for (auto entity : occView) {
        auto& world = occView.get<WorldTransformComponent>(entity);
        auto& renderer = occView.get<MeshRendererComponent>(entity);
        auto& occ = occView.get<OcclusionComponent>(entity);

        if (!renderer.model || occ.queryPending) continue;

        if (occ.lastQueryId == 0) {
            occ.lastQueryId = qm.GenQuery();
            AddQuery(occ.lastQueryId);
        }

        glm::mat4 modelMatrix = world.GetInterpolated(alpha);
        
        AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);
        
        glm::vec3 center = worldAABB.GetCenter();
        glm::vec3 size = worldAABB.GetSize();
        
        glm::mat4 cubeModel(1.0f);
        cubeModel = glm::translate(cubeModel, center);
        cubeModel = glm::scale(cubeModel, size * 0.5f);
        
        m_Shader->setMat4("model", cubeModel);

        qm.BeginQuery(Graphics::QueryType::SamplesPassed, occ.lastQueryId);
        m_Context->GetDrawContext().DrawElements(Graphics::Primitive::Triangles, 36, Graphics::DataType::UnsignedInt, 0);
        qm.EndQuery(Graphics::QueryType::SamplesPassed);
        
        occ.queryPending = true;
    }

    bm.BindVertexArray(0);
    rsm.ColorMask(true, true, true, true);
    rsm.DepthMask(true);
}
