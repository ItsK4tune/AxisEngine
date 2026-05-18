#pragma once

#include <entt/entt.hpp>
#include <string>
#include <vector>

enum class SystemCategory
{
    None = 0,
    Update = 1 << 0,
    RenderAlpha = 1 << 1,
    RenderMain = 1 << 2,
    RenderTransparent = 1 << 3,
    RenderUI = 1 << 4,
    Input = 1 << 5,
    Physics = 1 << 6,
    Script = 1 << 7,
    Audio = 1 << 8,
    AllRender = RenderAlpha | RenderMain | RenderTransparent | RenderUI,
    Core = 1 << 9,
    PostProcess = 1 << 10,
    RenderCapture = 1 << 11,
    EditorOverlay = 1 << 12
};

inline SystemCategory operator|(SystemCategory a, SystemCategory b)
{
    return static_cast<SystemCategory>(static_cast<int>(a) | static_cast<int>(b));
}

inline SystemCategory operator&(SystemCategory a, SystemCategory b)
{
    return static_cast<SystemCategory>(static_cast<int>(a) & static_cast<int>(b));
}

// Declares which engine services a system requires to initialize and run.
// SystemManager skips systems whose requirements are not met (e.g. headless mode).
enum class SystemRequirement : uint32_t
{
    None = 0,
    Graphics = 1 << 0,  // Needs IGraphicsContext
    Audio = 1 << 1,     // Needs IAudioEngine (real, not dummy)
    Input = 1 << 2,     // Needs IOHandler
};

inline SystemRequirement operator|(SystemRequirement a, SystemRequirement b)
{
    return static_cast<SystemRequirement>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline SystemRequirement operator&(SystemRequirement a, SystemRequirement b)
{
    return static_cast<SystemRequirement>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline SystemRequirement operator~(SystemRequirement a)
{
    return static_cast<SystemRequirement>(~static_cast<uint32_t>(a));
}

class IBaseSystem
{
public:
    virtual ~IBaseSystem() = default;

    virtual SystemCategory GetCategory() const
    {
        return SystemCategory::Update;
    }
    virtual SystemRequirement GetRequirements() const
    {
        return SystemRequirement::None;
    }

    virtual void Initialize()
    {
    }
    virtual void Shutdown()
    {
    }
    virtual void Reset()
    {
    }

    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsSleeping() const
    {
        return false;
    }
    virtual void SetSleep(bool sleep)
    {
    }
    virtual int GetPriority() const
    {
        return 0;
    }
    virtual std::string GetName() const = 0;
    virtual bool WantsFixedUpdate() const
    {
        return false;
    }
};
