#pragma once

#include <render/type/render_data.h>
#include <vector>

class RenderQueue
{
public:
    RenderQueue()
    {
        m_DeferredOpaqueQueue.reserve(16384);
        m_ForwardOpaqueQueue.reserve(2048);
        m_DepthOverlayQueue.reserve(512);
        m_TransparentQueue.reserve(2048);
        m_ShadowQueue.reserve(16384);
        m_Lights.reserve(256);
        m_ReflectionProbes.reserve(32);
        m_PlanarReflections.reserve(4);
        m_LightProbes.reserve(32);
    }

    void Clear()
    {
        m_DeferredOpaqueQueue.clear();
        m_ForwardOpaqueQueue.clear();
        m_DepthOverlayQueue.clear();
        m_TransparentQueue.clear();
        m_ShadowQueue.clear();
        m_Lights.clear();
        m_ReflectionProbes.clear();
        m_PlanarReflections.clear();
        m_LightProbes.clear();
    }
    void Swap(RenderQueue& other) noexcept
    {
        m_DeferredOpaqueQueue.swap(other.m_DeferredOpaqueQueue);
        m_ForwardOpaqueQueue.swap(other.m_ForwardOpaqueQueue);
        m_DepthOverlayQueue.swap(other.m_DepthOverlayQueue);
        m_TransparentQueue.swap(other.m_TransparentQueue);
        m_ShadowQueue.swap(other.m_ShadowQueue);
        m_Lights.swap(other.m_Lights);
        m_ReflectionProbes.swap(other.m_ReflectionProbes);
        m_PlanarReflections.swap(other.m_PlanarReflections);
        m_LightProbes.swap(other.m_LightProbes);
    }
    void AddDeferredOpaque(const RenderItem& item)
    {
        m_DeferredOpaqueQueue.push_back(item);
    }
    void AddForwardOpaque(const RenderItem& item)
    {
        m_ForwardOpaqueQueue.push_back(item);
    }
    void AddDepthOverlay(const RenderItem& item)
    {
        m_DepthOverlayQueue.push_back(item);
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
    void AddReflectionProbe(const RenderReflectionProbe& probe) { m_ReflectionProbes.push_back(probe); }
    void AddPlanarReflection(const RenderPlanarReflection& reflection) { m_PlanarReflections.push_back(reflection); }
    void AddLightProbe(const RenderLightProbe& probe) { m_LightProbes.push_back(probe); }

    void Sort(bool instanceBatchingEnabled = false);

    const std::vector<RenderItem>& GetDeferredOpaqueQueue() const
    {
        return m_DeferredOpaqueQueue;
    }
    const std::vector<RenderItem>& GetForwardOpaqueQueue() const
    {
        return m_ForwardOpaqueQueue;
    }
    const std::vector<RenderItem>& GetDepthOverlayQueue() const
    {
        return m_DepthOverlayQueue;
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
    std::vector<RenderLight>& GetLights()
    {
        return m_Lights;
    }
    const std::vector<RenderReflectionProbe>& GetReflectionProbes() const { return m_ReflectionProbes; }
    const std::vector<RenderPlanarReflection>& GetPlanarReflections() const { return m_PlanarReflections; }
    const std::vector<RenderLightProbe>& GetLightProbes() const { return m_LightProbes; }

private:
    std::vector<RenderItem> m_DeferredOpaqueQueue;
    std::vector<RenderItem> m_ForwardOpaqueQueue;
    std::vector<RenderItem> m_DepthOverlayQueue;
    std::vector<RenderItem> m_TransparentQueue;
    std::vector<RenderItem> m_ShadowQueue;
    std::vector<RenderLight> m_Lights;
    std::vector<RenderReflectionProbe> m_ReflectionProbes;
    std::vector<RenderPlanarReflection> m_PlanarReflections;
    std::vector<RenderLightProbe> m_LightProbes;
};
