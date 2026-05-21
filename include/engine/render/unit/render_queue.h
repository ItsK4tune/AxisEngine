#pragma once

#include <scene/logic/scene.h>
#include <glm/glm.hpp>
#include <vector>

class Shader;
class Model;
#include <core/unit/aabb.h>

class FrustumCuller;
struct AxisMaterialComponent;

#include <render/type/render_data.h>

class RenderQueue
{
public:
    RenderQueue()
    {
        m_DeferredOpaqueQueue.reserve(16384);
        m_ForwardOpaqueQueue.reserve(2048);
        m_TransparentQueue.reserve(2048);
        m_ShadowQueue.reserve(16384);
        m_Lights.reserve(256);
    }

    void Clear()
    {
        m_DeferredOpaqueQueue.clear();
        m_ForwardOpaqueQueue.clear();
        m_TransparentQueue.clear();
        m_ShadowQueue.clear();
        m_Lights.clear();
    }
    void AddDeferredOpaque(const RenderItem& item)
    {
        m_DeferredOpaqueQueue.push_back(item);
    }
    void AddForwardOpaque(const RenderItem& item)
    {
        m_ForwardOpaqueQueue.push_back(item);
    }
    void AddTransparent(const RenderItem& item)
    {
        m_TransparentQueue.push_back(item);
    }
    void AddShadow(const RenderItem& item)
    {
        m_ShadowQueue.push_back(item);
    }
    void AddLight(const RenderLight& light)
    {
        m_Lights.push_back(light);
    }
    void AddLight(RenderLight&& light)
    {
        m_Lights.push_back(std::move(light));
    }

    void Sort();

    const std::vector<RenderItem>& GetDeferredOpaqueQueue() const
    {
        return m_DeferredOpaqueQueue;
    }
    const std::vector<RenderItem>& GetForwardOpaqueQueue() const
    {
        return m_ForwardOpaqueQueue;
    }
    const std::vector<RenderItem>& GetTransparentQueue() const
    {
        return m_TransparentQueue;
    }
    const std::vector<RenderItem>& GetShadowQueue() const
    {
        return m_ShadowQueue;
    }
    const std::vector<RenderLight>& GetLights() const
    {
        return m_Lights;
    }

private:
    std::vector<RenderItem> m_DeferredOpaqueQueue;
    std::vector<RenderItem> m_ForwardOpaqueQueue;
    std::vector<RenderItem> m_TransparentQueue;
    std::vector<RenderItem> m_ShadowQueue;
    std::vector<RenderLight> m_Lights;
};
