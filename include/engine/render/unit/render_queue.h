#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>
#include <vector>

class Shader;
class Model;
#include <core/unit/aabb.h>
class FrustumCuller;
struct AxisMaterialComponent;

#include <render/type/render_data.h>

class RenderQueue {
public:
    void Clear() { m_OpaqueQueue.clear(); m_TransparentQueue.clear(); m_ShadowQueue.clear(); m_Lights.clear(); }
    void AddOpaque(const RenderItem& item) { m_OpaqueQueue.push_back(item); }
    void AddTransparent(const RenderItem& item) { m_TransparentQueue.push_back(item); }
    void AddShadow(const RenderItem& item) { m_ShadowQueue.push_back(item); }
    void AddLight(const RenderLight& light) { m_Lights.push_back(light); }
    void AddLight(RenderLight&& light) { m_Lights.push_back(std::move(light)); }

    void Sort();

    const std::vector<RenderItem>& GetOpaqueQueue() const { return m_OpaqueQueue; }
    const std::vector<RenderItem>& GetTransparentQueue() const { return m_TransparentQueue; }
    const std::vector<RenderItem>& GetShadowQueue() const { return m_ShadowQueue; }
    const std::vector<RenderLight>& GetLights() const { return m_Lights; }

private:
    std::vector<RenderItem> m_OpaqueQueue;
    std::vector<RenderItem> m_TransparentQueue;
    std::vector<RenderItem> m_ShadowQueue;
    std::vector<RenderLight> m_Lights;
};