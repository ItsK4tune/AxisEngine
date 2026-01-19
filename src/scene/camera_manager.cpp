#include <scene/camera_manager.h>
#include <scene/scene.h>
#include <app/application.h>
#include <script/script_registry.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

CameraManager::CameraManager(Scene& scene)
    : m_Scene(scene)
{
}

CameraManager::~CameraManager()
{
}

entt::entity CameraManager::GetPrimaryCamera() const
{
    auto view = m_Scene.registry.view<CameraComponent>();
    
    for (auto entity : view)
    {
        const auto& camera = view.get<CameraComponent>(entity);
        if (camera.isPrimary)
        {
            return entity;
        }
    }
    
    return entt::null;
}

void CameraManager::SetPrimaryCamera(entt::entity entity)
{
    // Clear all other primary flags
    auto view = m_Scene.registry.view<CameraComponent>();
    for (auto e : view)
    {
        auto& camera = view.get<CameraComponent>(e);
        camera.isPrimary = false;
    }
    
    // Set new primary
    if (m_Scene.registry.valid(entity) && m_Scene.registry.all_of<CameraComponent>(entity))
    {
        auto& camera = m_Scene.registry.get<CameraComponent>(entity);
        camera.isPrimary = true;
    }
}

std::vector<entt::entity> CameraManager::GetAllCameras() const
{
    std::vector<entt::entity> cameras;
    auto view = m_Scene.registry.view<CameraComponent>();
    
    for (auto entity : view)
    {
        cameras.push_back(entity);
    }
    
    return cameras;
}

void CameraManager::EnsurePrimaryCamera(Application* app)
{
    if (GetPrimaryCamera() != entt::null)
        return;

    std::cout << "[CameraManager] WARNING: No Active Camera found in scene! Creating Default Spectator Camera." << std::endl;
    CreateDefaultSpectatorCamera(app);
}

entt::entity CameraManager::CreateCamera(const glm::vec3& position, float fov, float yaw, float pitch, bool isPrimary)
{
    entt::entity entity = m_Scene.createEntity();
    
    // Set transform
    auto& transform = m_Scene.registry.emplace<TransformComponent>(entity);
    transform.position = position;
    
    // Set camera
    auto& camera = m_Scene.registry.emplace<CameraComponent>(entity);
    camera.isPrimary = isPrimary;
    camera.fov = fov;
    camera.yaw = yaw;
    camera.pitch = pitch;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;
    camera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    return entity;
}

entt::entity CameraManager::CreateDefaultSpectatorCamera(Application* app)
{
    entt::entity camEntity = CreateCamera(glm::vec3(0.0f, 2.0f, 10.0f), 45.0f, -90.0f, 0.0f, true);
    
    // Add info component
    m_Scene.registry.emplace<InfoComponent>(camEntity, "Default Spectator Camera", "Default");
    
    // Add default camera controller script
    std::string scriptName = "DefaultCameraController";
    Scriptable* scriptInstance = ScriptRegistry::Instance().Create(scriptName);
    
    if (scriptInstance)
    {
        auto& scriptComp = m_Scene.registry.emplace<ScriptComponent>(camEntity);
        scriptComp.instance = scriptInstance;
        scriptComp.InstantiateScript = [scriptName]() { return ScriptRegistry::Instance().Create(scriptName); };
        scriptComp.DestroyScript = [](ScriptComponent* nsc) { delete nsc->instance; nsc->instance = nullptr; };
        
        scriptComp.instance->Init(camEntity, &m_Scene, app);
        scriptComp.instance->OnCreate();
        std::cout << "[CameraManager] Attached 'DefaultCameraController' (Engine Fallback) to default camera." << std::endl;
    }
    else
    {
        std::cout << "[CameraManager] 'DefaultCameraController' script not found! Make sure it is compiled." << std::endl;
    }
    
    return camEntity;
}

glm::mat4 CameraManager::GetViewMatrix(entt::entity camera) const
{
    if (!m_Scene.registry.valid(camera) || !m_Scene.registry.all_of<CameraComponent>(camera))
        return glm::mat4(1.0f);
    
    const auto& cam = m_Scene.registry.get<CameraComponent>(camera);
    return cam.GetViewMatrix();
}

glm::mat4 CameraManager::GetProjectionMatrix(entt::entity camera, float aspectRatio) const
{
    if (!m_Scene.registry.valid(camera) || !m_Scene.registry.all_of<CameraComponent>(camera))
        return glm::mat4(1.0f);
    
    const auto& cam = m_Scene.registry.get<CameraComponent>(camera);
    return glm::perspective(glm::radians(cam.fov), aspectRatio, cam.nearPlane, cam.farPlane);
}

glm::vec3 CameraManager::GetCameraPosition(entt::entity camera) const
{
    if (!m_Scene.registry.valid(camera) || !m_Scene.registry.all_of<TransformComponent>(camera))
        return glm::vec3(0.0f);
    
    const auto& transform = m_Scene.registry.get<TransformComponent>(camera);
    return transform.position;
}

glm::vec3 CameraManager::GetCameraDirection(entt::entity camera) const
{
    if (!m_Scene.registry.valid(camera) || !m_Scene.registry.all_of<CameraComponent>(camera))
        return glm::vec3(0.0f, 0.0f, -1.0f);
    
    const auto& cam = m_Scene.registry.get<CameraComponent>(camera);
    return glm::normalize(cam.front);
}
