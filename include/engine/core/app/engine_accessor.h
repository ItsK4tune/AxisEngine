#pragma once

#include <core/interface/i_base_system.h>
#include <core/interface/i_localization_service.h>
#include <ecs/interface/i_system_registry.h>
#include <platform/interface/cursor_mode.h>
#include <render/type/graphics_types.h>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

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
struct SceneRecord;
struct AppConfig;
struct DataNode;

class EngineAccessor
{
public:
    virtual ~EngineAccessor() = default;

    template <typename T>
    T& Get() const
    {
        if (auto* service = Resolve<T>())
            return *service;
        throw std::runtime_error(std::string("Engine service not registered: ") + typeid(T).name());
    }

    template <typename T>
    T* Resolve() const
    {
        return static_cast<T*>(ResolveService(std::type_index(typeid(T))));
    }

    template <typename T>
    bool Has() const
    {
        return Resolve<T>() != nullptr;
    }

    template <typename T>
    T& GetSystem() const
    {
        auto* sys = dynamic_cast<T*>(ResolveSystem(std::type_index(typeid(T))));
        if (!sys)
            throw std::runtime_error("System not found: " + std::string(typeid(T).name()));
        return *sys;
    }

    Scene& GetScene() const;
    void LoadScene(const std::string& path, bool persistent = false);
    void QueueLoadScene(const std::string& path, bool persistent = false);
    void UnloadScene(const std::string& path);
    void QueueUnloadScene(const std::string& path);
    void UnloadScene(const SceneRecord* rec);
    void ChangeScene(const std::string& path);
    void QueueChangeScene(const std::string& path);
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
    void SetDataNode(const std::string& key, const DataNode& data);
    DataNode GetDataNode(const std::string& key) const;
    bool HasDataNode(const std::string& key) const;
    void RemoveDataNode(const std::string& key);
    bool LoadConfig(const std::string& path);
    bool SaveConfig(const std::string& path);
    bool GetAction(const std::string& name) const;
    bool GetActionDown(const std::string& name) const;
    bool GetActionUp(const std::string& name) const;
    float GetAxis(const std::string& name) const;
    void SetCursorMode(CursorMode mode);

    void LoadLanguage(const std::string& path, const std::string& name = "");
    void SetLanguage(const std::string& name);
    std::string GetLanguage() const;
    std::string GetTranslation(const std::string& key) const;

    template <typename... Args>
    std::string GetTranslation(const std::string& key, Args... args) const
    {
        if (auto* loc = Resolve<ILocalizationService>())
        {
            std::vector<std::string> values;
            values.reserve(sizeof...(Args));
            (values.push_back(ToString(args)), ...);
            return loc->Format(key, values);
        }
        return "[MISSING: " + key + "]";
    }

    void EnableSystem(const std::string& systemName, bool enable);
    void EnableSystem(SystemId systemId, bool enable);
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
    void CreateHingeConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB,
                               const glm::vec3& axisA, const glm::vec3& axisB);
    void CreatePointToPointConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB);
    void CreateFixedConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA, const glm::vec3& pivotB,
                               const glm::quat& rotA, const glm::quat& rotB);

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
    void ConfigurePostProcessing(bool hdr, bool bloom, float threshold, float intensity, float radius, float exposure,
                                 float gamma, int tonemappingMode);

    void GetCameraRenderState(glm::vec3& outPos, glm::mat4& outView, glm::mat4& outProj, float& outNear, float& outFar);
    void SetCameraRenderState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane,
                              float farPlane);
    void DrawEntityMesh(Entity entity, const std::string& shaderName, const glm::mat4& customWorldTransform,
                        const glm::vec4& color, float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);

    // High-level ECS Queries
    std::vector<Entity> GetEntitiesWithName(const std::string& name) const;
    std::vector<Entity> GetEntitiesWithNamePrefix(const std::string& prefix) const;
    std::vector<Entity> GetCameraEntities() const;
    size_t GetEntityCount() const;
    void UpdateNavMeshHeightsAndTags(
        std::function<void(const glm::vec3& pos, glm::vec3& outPos, std::string& outTag)> modifier);

    void SetActiveScene(Scene* scene)
    {
        m_ActiveScene = scene;
    }

protected:
    Scene* m_ActiveScene = nullptr;

private:
    template <typename T>
    static std::string ToString(const T& value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    void* ResolveService(std::type_index type) const;
    IBaseSystem* ResolveSystem(std::type_index type) const;
};
