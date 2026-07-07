#pragma once

#include <core/logic/localization_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <platform/interface/cursor_mode.h>
#include <ecs/logic/system_manager.h>
#include <render/type/graphics_types.h>
#include <string>
#include <vector>
#include <functional>

struct Scene;
class Entity;
class SceneManager;
class ResourceManager;
class AudioService;
class IOHandler;
class InputManager;
class KeyboardManager;
class MouseManager;
class RuntimeCore;
class SystemManager;
struct SceneRecord;
struct AppConfig;

class EngineAccessor
{
public:
    virtual ~EngineAccessor() = default;

    template <typename T>
    T& Get() const
    {
        return ServiceLocator::Instance().Require<T>();
    }

    template <typename T>
    T* Resolve() const
    {
        return ServiceLocator::Instance().Resolve<T>();
    }

    template <typename T>
    bool Has() const
    {
        return ServiceLocator::Instance().Has<T>();
    }

    template <typename T>
    T& GetSystem() const
    {
        auto* sys = Get<SystemManager>().GetSystem<T>();
        if (!sys)
            throw std::runtime_error("System not found: " + std::string(typeid(T).name()));
        return *sys;
    }

    Scene& GetScene() const;
    void LoadScene(const std::string& path, bool persistent = false);
    void QueueLoadScene(const std::string& path, bool persistent = false);
    void UnloadScene(const std::string& path);
    void UnloadScene(const SceneRecord* rec);
    void ChangeScene(const std::string& path);
    void PopScene();
    void QueuePopScene();
    bool IsSceneLoaded(const std::string& path);
    void LogAllScenes();
    bool SaveScene(const std::string& path, const std::string& sceneName = "");
    std::vector<const SceneRecord*> GetScenes();

    bool LoadInputBindings(const std::string& path);
    bool SaveInputBindings(const std::string& path);
    bool LoadDataNodes(const std::string& path);
    bool SaveDataNodes(const std::string& path);
    bool LoadConfig(const std::string& path);
    bool SaveConfig(const std::string& path);
    bool GetAction(const std::string& name) const;
    bool GetActionDown(const std::string& name) const;
    bool GetActionUp(const std::string& name) const;
    void SetCursorMode(CursorMode mode);

    void LoadLanguage(const std::string& path, const std::string& name = "");
    void SetLanguage(const std::string& name);
    std::string GetLanguage() const;
    std::string GetTranslation(const std::string& key) const;

    template <typename... Args>
    std::string GetTranslation(const std::string& key, Args... args) const
    {
        if (auto* loc = Resolve<LocalizationSystem>())
        {
            return loc->GetFormat(key, args...);
        }
        return "[MISSING: " + key + "]";
    }

    void EnableSystem(const std::string& systemName, bool enable);
    void EnablePhysics(bool enable)
    {
        EnableSystem("PhysicsSystem", enable);
    }
    void EnableRender(bool enable)
    {
        EnableSystem("RenderSystem", enable);
    }
    void EnableAudio(bool enable)
    {
        EnableSystem("AudioSystem", enable);
    }
    void EnableScript(bool enable)
    {
        EnableSystem("ScriptableSystem", enable);
    }
    void EnableAnimation(bool enable)
    {
        EnableSystem("AnimationSystem", enable);
    }
    void EnableVideo(bool enable)
    {
        EnableSystem("VideoSystem", enable);
    }
    void EnableUIRender(bool enable)
    {
        EnableSystem("UIRenderSystem", enable);
    }
    void EnableParticle(bool enable)
    {
        EnableSystem("ParticleSystem", enable);
    }
    void EnableSkybox(bool enable)
    {
        EnableSystem("SkyboxRenderSystem", enable);
    }
    void EnableNavigation(bool enable)
    {
        EnableSystem("NavigationSystem", enable);
    }
    void EnableLogic(bool enable);

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    AppConfig GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    // High-level Physics / Collisions API
    void SetPhysicsGravity(const glm::vec3& gravity);
    void SetPhysicsSolverIterations(int iterations);
    void IgnoreTagCollision(const std::string& tag1, const std::string& tag2);
    void ForcePhysicsUpdate(float dt);
    void CreateHingeConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB, const glm::vec3& axisA, const glm::vec3& axisB);
    void CreatePointToPointConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB);
    void CreateFixedConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB, const glm::quat& rotA, const glm::quat& rotB);

    // High-level Rendering / Stencil API
    void ClearStencilBuffer();
    void ClearDepthBuffer();
    void SetRenderStateEnabled(ServerCapability capability, bool enable);
    void SetStencilMask(uint32_t mask);
    void SetStencilFunc(CompareFunc func, int ref, uint32_t mask);
    void SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    void SetDepthFunc(CompareFunc func);
    void SetColorWriteMask(bool r, bool g, bool b, bool a);
    void SetDepthWriteMask(bool enable);
    void ConfigurePostProcessing(bool hdr, bool bloom, float threshold, float intensity, float radius, float exposure, float gamma, int tonemappingMode);

    void GetCameraRenderState(glm::vec3& outPos, glm::mat4& outView, glm::mat4& outProj, float& outNear, float& outFar);
    void SetCameraRenderState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane, float farPlane);
    void DrawEntityMesh(Entity entity, const std::string& shaderName, const glm::mat4& customWorldTransform, const glm::vec4& color, float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);

    // High-level ECS Queries
    std::vector<Entity> GetEntitiesWithName(const std::string& name) const;
    std::vector<Entity> GetEntitiesWithNamePrefix(const std::string& prefix) const;
    std::vector<Entity> GetCameraEntities() const;
    size_t GetEntityCount() const;
    void UpdateNavMeshHeightsAndTags(std::function<void(const glm::vec3& pos, glm::vec3& outPos, std::string& outTag)> modifier);

    void SetActiveScene(Scene* scene)
    {
        m_ActiveScene = scene;
    }

protected:
    Scene* m_ActiveScene = nullptr;
};
