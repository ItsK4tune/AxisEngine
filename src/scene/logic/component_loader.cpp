#include <scene/logic/component_loader.h>
#include <audio/logic/audio_service.h>
#include <core/app/application.h>
#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/fragment_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/network_components.h>
#include <ecs/unit/ui_components.h>
#include <navigation/unit/pathfollower_component.h>
#include <physics/logic/physics_loader.h>
#include <resource/unit/animator.h>
#include <algorithm>
#include <iostream>

std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> ComponentLoader::s_Factories;
std::unordered_map<std::string, ComponentLoaderFunc> ComponentLoader::s_Loaders;

void ComponentLoader::RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory)
{
    s_Factories[type] = std::move(factory);
}

void ComponentLoader::RegisterLoader(const std::string& type, ComponentLoaderFunc func)
{
    s_Loaders[type] = std::move(func);
}

bool ComponentLoader::Load(const std::string& type, Scene& scene, entt::entity entity, const YAMLNode& node,
                           ResourceManager& res, IPhysicsWorld* phys)
{
    if (auto it = s_Loaders.find(type); it != s_Loaders.end())
    {
        it->second(scene, entity, node, res, phys);
        return true;
    }
    if (auto it = s_Factories.find(type); it != s_Factories.end())
    {
        it->second->Load(scene, entity, node, res, phys);
        return true;
    }
    return false;
}

void ComponentLoader::InitializeDefaultLoaders()
{
    static bool s_Initialized = false;
    if (s_Initialized)
        return;
    s_Initialized = true;

    RegisterLoader("Renderer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadRenderer(s, e, n, r);
    });
    RegisterLoader("Animator", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadAnimator(s, e, n, r);
    });
    RegisterLoader("Camera", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadCamera(s, e, n);
    });

    // Standardized Lighting Names
    RegisterLoader("DirectionalLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                          IPhysicsWorld* p) { LoadLightDir(s, e, n); });
    RegisterLoader("PointLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightPoint(s, e, n);
    });
    RegisterLoader("SpotLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightSpot(s, e, n);
    });

    // Legacy Lighting Names (Backup)
    RegisterLoader("LightDir", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightDir(s, e, n);
    });
    RegisterLoader("LightPoint", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightPoint(s, e, n);
    });
    RegisterLoader("LightSpot", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightSpot(s, e, n);
    });

    RegisterLoader("UITransform", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadUITransform(s, e, n); });
    RegisterLoader("UIRenderer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadUIRenderer(s, e, n, r);
    });
    RegisterLoader("UIText", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadUIText(s, e, n, r);
    });
    RegisterLoader("SkyboxRenderer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadSkyboxRenderer(s, e, n, r); });
    RegisterLoader("ReflectionProbe", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                         IPhysicsWorld* p) { LoadReflectionProbe(s, e, n, r); });
    RegisterLoader("PostProcess", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadPostProcess(s, e, n, r); });
    RegisterLoader("VideoPlayer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadVideoPlayer(s, e, n); });
    RegisterLoader("Animation", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadAnimator(s, e, n, r);
    });
    RegisterLoader("ParticleEmitter", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                         IPhysicsWorld* p) { LoadParticleEmitter(s, e, n, r); });
    RegisterLoader("Material", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadMaterial(s, e, n, r);
    });
    RegisterLoader("LOD", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLOD(s, e, n, r);
    });

    // New Modular Physics Components
    RegisterLoader("RigidShape", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        if (p)
            PhysicsLoader::LoadRigidShape(s, e, n, *p);
    });
    RegisterLoader("RigidBody", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        if (p)
            PhysicsLoader::LoadRigidBody(s, e, n, *p);
    });

    RegisterLoader("CharacterController",
                   [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
                       if (p)
                           PhysicsLoader::LoadCharacterController(s, e, n, *p);
                   });
    RegisterLoader("Transform", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadTransform(s, e, n);
    });
    RegisterLoader("PathFollower", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { ComponentLoader::LoadPathFollower(s, e, n); });
    RegisterLoader("Decal", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadDecal(s, e, n, r);
    });
    RegisterLoader("UIFlex", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadUIFlex(s, e, n);
    });
    RegisterLoader("Reflective", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadReflective(s, e, n, r);
    });
    RegisterLoader("AudioSource", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadAudioSource(s, e, n, r); });
    RegisterLoader("Audio", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadAudioSource(s, e, n, r);
    });
    RegisterLoader("Fragment", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadFragment(s, e, n);
    });
    RegisterLoader("PlanarReflection", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                          IPhysicsWorld* p) { LoadPlanarReflection(s, e, n); });
    RegisterLoader("LightProbe", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadLightProbe(s, e, n);
    });
    RegisterLoader("Terrain", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadTerrain(s, e, n, r);
    });
    RegisterLoader("Network", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
        LoadNetwork(s, e, n);
    });
}

// Helper to serialize YAMLNode back to string for overrides
static std::string SerializeYAML(const YAMLNode& node, int indent = 0)
{
    std::string result = "";
    std::string indentStr(indent * 2, ' ');
    for (const auto& child : node.children)
    {
        result += indentStr + child.key + ": " + child.value + "\n";
        if (!child.children.empty())
        {
            result += SerializeYAML(child, indent + 1);
        }
    }
    return result;
}

static void RedundantLoader(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    auto& anim = scene.registry.emplace<AnimationComponent>(entity);
    std::stringstream ss(node.GetChildValue("Animation"));
    std::string a;
    while (ss >> a) anim.animations.push_back(a);
}

void ComponentLoader::LoadRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(
        node, {"Model", "Shader", "Order", "Color", "CastShadow", "ReceiveShadow", "IgnoreDepth", "RenderMode"},
        "Renderer");

    std::string modelName = node.GetChildValue("Model");
    std::string shaderName = node.GetChildValue("Shader");
    int order = std::stoi(node.GetChildValue("Order", "0"));
    bool castShadow =
        node.GetChildValue("CastShadow", "1") == "1" || node.GetChildValue("CastShadow", "true") == "true";
    bool receiveShadow =
        node.GetChildValue("ReceiveShadow", "1") == "1" || node.GetChildValue("ReceiveShadow", "true") == "true";
    bool ignoreDepth =
        node.GetChildValue("IgnoreDepth", "0") == "1" || node.GetChildValue("IgnoreDepth", "false") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1 1"));
    float cr = 1, cg = 1, cb = 1, ca = 1;
    colorSS >> cr >> cg >> cb >> ca;
    glm::vec4 color(cr, cg, cb, ca);

    std::string entityName = "Unknown";
    if (scene.registry.all_of<InfoComponent>(entity))
        entityName = scene.registry.get<InfoComponent>(entity).name;

    LOGGER_INFO("ComponentLoader") << "[DEBUG] Renderer load on '" << entityName << "': Color(" << cr << ", " << cg
                                   << ", " << cb << ", " << ca << ")";

    auto& r = scene.registry.emplace<MeshRendererComponent>(entity);
    if (modelName.empty())
    {
        std::string keys = "";
        for (auto& c : node.children) keys += c.key + ", ";
        LOGGER_WARN("ComponentLoader") << "Renderer component on entity '" << entityName
                                       << "' missing 'Model' field. Found keys: " << keys;
    }
    else if (shaderName.empty())
    {
        shaderName = "forward_unlit";
        LOGGER_INFO("ComponentLoader") << "Renderer component on entity '" << entityName
                                       << "' missing 'Shader' field. Falling back to " << shaderName << ".";
    }

    r.model = res.GetModelAuto(modelName, false);
    r.shader = res.GetShader(shaderName);
    r.shaderName = shaderName;

    r.order = order;
    r.castShadow = castShadow;
    r.receiveShadow = receiveShadow;
    r.ignoreDepth = ignoreDepth;
    r.color = color;
    r.renderMode = (RenderMode)std::stoi(node.GetChildValue("RenderMode", "0"));

    if (!r.model)
        LOGGER_WARN("ComponentLoader") << "Renderer model not found on entity '" << entityName << "': " << modelName;
    if (r.shader.expired())
        LOGGER_WARN("ComponentLoader") << "Renderer shader not found on entity '" << entityName << "': " << shaderName;
}

void ComponentLoader::LoadAnimator(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Animation", "Speed", "StartTime", "Rate", "BlendFactor"}, "Animator");

    auto& a = scene.registry.emplace<AnimationComponent>(entity);

    std::string animNames = node.GetChildValue("Animation");
    if (animNames.empty())
    {
        LOGGER_WARN("ComponentLoader") << "Animator component missing 'Animation' field";
    }
    else
    {
        std::stringstream ss(animNames);
        std::string name;
        while (ss >> name)
        {
            a.animations.push_back(name);
        }
    }

    a.speed = std::stof(node.GetChildValue("Speed", "1.0"));
    a.startTime = std::stof(node.GetChildValue("StartTime", "0.0"));
    a.rate = std::stof(node.GetChildValue("Rate", "30.0"));
    a.blendFactor = std::stof(node.GetChildValue("BlendFactor", "0.0"));

    if (a.speed < 0.0f)
        LOGGER_WARN("ComponentLoader") << "Animator Speed should not be negative: " << a.speed;
    if (a.rate <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "Animator Rate should be positive: " << a.rate;

    if (!a.animations.empty())
    {
        auto getAnim = [&](const std::string& nameOrPath) -> std::shared_ptr<Animation> {
            return res.GetAnimation(nameOrPath);
        };

        auto firstAnim = getAnim(a.animations[0]);
        if (firstAnim)
        {
            a.animator = std::make_shared<Animator>(firstAnim);
            a.animator->AddAnimation(a.animations[0], firstAnim);
            a.animator->SetSpeed(a.speed);
            a.animator->SetTime(a.startTime);
            a.animator->SetUpdateRate(a.rate);
            a.animator->SetBlendFactor(a.blendFactor);

            if (scene.registry.all_of<MeshRendererComponent>(entity))
            {
                auto& mrc = scene.registry.get<MeshRendererComponent>(entity);
                if (mrc.model && mrc.model->IsStatic())
                {
                    LOGGER_WARN("ComponentLoader")
                        << "Entity has Animator but its Model is STATIC! Animations will not play correctly. Entity: "
                        << (scene.registry.all_of<InfoComponent>(entity)
                                ? scene.registry.get<InfoComponent>(entity).name
                                : "Unknown");
                }
            }

            for (size_t i = 1; i < a.animations.size(); ++i)
            {
                auto extraAnim = getAnim(a.animations[i]);
                if (extraAnim)
                {
                    a.animator->AddAnimation(a.animations[i], extraAnim);
                }
            }
        }
        else
        {
            LOGGER_WARN("ComponentLoader") << "Default animation not found: " << a.animations[0];
        }
    }
}

void ComponentLoader::LoadPostProcess(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Active", "Effects"}, "PostProcess");
    auto& pp = scene.registry.emplace<PostProcessComponent>(entity);
    pp.enabled = node.GetChildValue("Active", "true") == "true";

    std::string effectsStr = node.GetChildValue("Effects");
    if (!effectsStr.empty())
    {
        std::stringstream ss(effectsStr);
        std::string segment;
        while (ss >> segment)
        {
            std::vector<std::string> parts;
            std::stringstream pss(segment);
            std::string part;
            while (std::getline(pss, part, ':'))
            {
                parts.push_back(part);
            }

            PostProcessComponent::Effect effect;
            if (parts.size() >= 1)
                effect.shaderName = parts[0];
            if (parts.size() >= 2)
            {
                try
                {
                    effect.priority = std::stoi(parts[1]);
                }
                catch (...)
                {
                }
            }
            if (parts.size() >= 6)
            {
                try
                {
                    effect.x = std::stoi(parts[2]);
                    effect.y = std::stoi(parts[3]);
                    effect.w = std::stoi(parts[4]);
                    effect.h = std::stoi(parts[5]);
                }
                catch (...)
                {
                }
            }
            if (parts.size() >= 7)
            {
                effect.affectUI = (parts[6] == "1");
            }
            pp.effects.push_back(effect);
        }
    }
}

void ComponentLoader::LoadReflective(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Active", "Reflectivity", "FresnelPower", "FresnelBias", "Probe"}, "Reflective");
    auto& ref = scene.registry.emplace<ReflectiveComponent>(entity);
    ref.enabled = node.GetChildValue("Active", "true") == "true";
    ref.reflectivity = std::stof(node.GetChildValue("Reflectivity", "1.0"));
    ref.fresnelPower = std::stof(node.GetChildValue("FresnelPower", "5.0"));
    ref.fresnelBias = std::stof(node.GetChildValue("FresnelBias", "0.04"));
    ref.targetProbe = node.GetChildValue("Probe", "");
}

void ComponentLoader::LoadCamera(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Primary", "FOV", "Yaw", "Pitch", "Near", "Far", "AspectRatio"}, "Camera");

    auto& c = scene.registry.emplace<CameraComponent>(entity);
    c.isPrimary = node.GetChildValue("Primary", "1") == "1" || node.GetChildValue("Primary", "true") == "true";

    c.aspectRatio = std::stof(node.GetChildValue("AspectRatio", "0.0"));

    c.fov = std::stof(node.GetChildValue("FOV", "45.0"));
    if (c.fov <= 0.0f || c.fov >= 180.0f)
        LOGGER_WARN("ComponentLoader") << "Camera FOV out of bounds (0-180): " << c.fov;

    float yaw = std::stof(node.GetChildValue("Yaw", "-90.0"));
    float pitch = std::stof(node.GetChildValue("Pitch", "0.0"));
    if (pitch < -89.0f || pitch > 89.0f)
        LOGGER_WARN("ComponentLoader") << "Camera Pitch out of bounds (-89 to 89): " << pitch;

    // Convert Euler to Quaternion and update/emplace RotationComponent
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(glm::normalize(front), worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, glm::normalize(front)));

    glm::quat rotation = glm::quatLookAt(glm::normalize(front), up);

    auto& rotComp = scene.registry.get_or_emplace<RotationComponent>(entity);
    rotComp.value = rotation;
    rotComp.prev = rotation;

    c.nearPlane = std::stof(node.GetChildValue("Near", "0.1"));
    c.farPlane = std::stof(node.GetChildValue("Far", "1000.0"));

    if (c.nearPlane <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane should be > 0";
    if (c.nearPlane >= c.farPlane)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane must be less than Far plane";

    if (c.isPrimary)
        EntityManager::SetActiveCamera(scene, entity);
}

void ComponentLoader::LoadLightDir(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "Ambient", "Diffuse", "Specular"},
                              "LightDir");

    auto& l = scene.registry.emplace<DirectionalLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = std::stof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightDir Intensity should not be negative: " << l.intensity;

    l.ambient = std::stof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = std::stof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = std::stof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadLightPoint(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "CastShadow", "Color", "Intensity", "Radius", "Constant", "Linear",
                               "Quadratic", "Ambient", "Diffuse", "Specular"},
                              "LightPoint");

    auto& l = scene.registry.emplace<PointLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = std::stof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightPoint Intensity should not be negative: " << l.intensity;

    l.radius = std::stof(node.GetChildValue("Radius", "10.0"));
    if (l.radius <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightPoint Radius should be positive: " << l.radius;

    l.constant = std::stof(node.GetChildValue("Constant", "1.0"));
    l.linear = std::stof(node.GetChildValue("Linear", "0.09"));
    l.quadratic = std::stof(node.GetChildValue("Quadratic", "0.032"));

    l.ambient = std::stof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = std::stof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = std::stof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadLightSpot(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "CastShadow", "Color", "Intensity", "CutOff", "OuterCutOff", "Constant",
                               "Linear", "Quadratic", "Ambient", "Diffuse", "Specular"},
                              "LightSpot");

    auto& l = scene.registry.emplace<SpotLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = std::stof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot Intensity should not be negative: " << l.intensity;

    float cutOffAng = std::stof(node.GetChildValue("CutOff", "12.5"));
    float outerCutOffAng = std::stof(node.GetChildValue("OuterCutOff", "17.5"));

    if (cutOffAng < 0.0f || cutOffAng > 90.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot CutOff out of bounds (0-90): " << cutOffAng;
    if (outerCutOffAng < 0.0f || outerCutOffAng > 90.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot OuterCutOff out of bounds (0-90): " << outerCutOffAng;
    if (cutOffAng > outerCutOffAng)
        LOGGER_WARN("ComponentLoader") << "LightSpot CutOff should be less than or equal to OuterCutOff";

    l.cutOff = glm::cos(glm::radians(cutOffAng));
    l.outerCutOff = glm::cos(glm::radians(outerCutOffAng));

    l.constant = std::stof(node.GetChildValue("Constant", "1.0"));
    l.linear = std::stof(node.GetChildValue("Linear", "0.09"));
    l.quadratic = std::stof(node.GetChildValue("Quadratic", "0.032"));

    l.ambient = std::stof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = std::stof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = std::stof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadUITransform(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& ui = scene.registry.emplace<UITransformComponent>(entity);

    auto parseVec2Percent = [](const std::string& str, glm::vec2& outVec, glm::bvec2& outPercent,
                               const glm::vec2& defaultVec) {
        if (str.empty())
        {
            outVec = defaultVec;
            outPercent = glm::bvec2(false);
            return;
        }
        std::stringstream ss(str);
        std::string xStr, yStr;
        ss >> xStr >> yStr;

        auto parseComp = [](const std::string& s, float& v, bool& p) {
            if (s.empty())
                return;
            std::string t = s;
            if (t.back() == '%')
            {
                p = true;
                t.pop_back();
            }
            else
                p = false;
            try
            {
                v = std::stof(t);
            }
            catch (...)
            {
            }
        };

        parseComp(xStr, outVec.x, outPercent.x);
        parseComp(yStr, outVec.y, outPercent.y);
    };

    parseVec2Percent(node.GetChildValue("Position"), ui.position, ui.positionIsPercent, glm::vec2(0.0f));
    parseVec2Percent(node.GetChildValue("Size"), ui.size, ui.sizeIsPercent, glm::vec2(100.0f));

    ui.zIndex = std::stoi(node.GetChildValue("ZOrder", "0"));
    std::string zLabel = node.GetChildValue("zIndex");
    if (!zLabel.empty())
        ui.zIndex = std::stoi(zLabel);

    parseVec2Percent(node.GetChildValue("anchorMin"), ui.anchorMin, ui.anchorMinIsPercent, glm::vec2(0.5f));
    parseVec2Percent(node.GetChildValue("anchorMax"), ui.anchorMax, ui.anchorMaxIsPercent, glm::vec2(0.5f));
    parseVec2Percent(node.GetChildValue("offsetMin"), ui.offsetMin, ui.offsetMinIsPercent, glm::vec2(-50.0f));
    parseVec2Percent(node.GetChildValue("offsetMax"), ui.offsetMax, ui.offsetMaxIsPercent, glm::vec2(50.0f));

    std::stringstream pivotSS(node.GetChildValue("pivot", "0.5 0.5"));
    pivotSS >> ui.pivot.x >> ui.pivot.y;
}

void ComponentLoader::LoadUIRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Color", "color", "Texture", "texture", "Shader", "shader"}, "UIRenderer");
    auto& ui = scene.registry.emplace<UIRendererComponent>(entity);

    std::stringstream colorSS(node.GetChildValue("color", "1 1 1 1"));
    if (node.GetChildValue("color").empty())
        colorSS.str(node.GetChildValue("Color", "1 1 1 1"));
    colorSS >> ui.color.r >> ui.color.g >> ui.color.b >> ui.color.a;

    auto StripQuotes = [](std::string s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.length() - 2);
        return s;
    };

    std::string textureName = StripQuotes(node.GetChildValue("texture"));
    if (textureName.empty())
        textureName = StripQuotes(node.GetChildValue("Texture"));

    if (!textureName.empty())
    {
        std::shared_ptr<Texture> tex = res.GetTextureAuto(textureName);
        std::string finalName = textureName;

        if (!res.GetUIModel(finalName))
            res.CreateUIModel(finalName, ::UIType::Texture);
        auto model = res.GetUIModel(finalName);
        if (tex && model)
        {
            model->SetTexture(tex->id);
            ui.texture = tex;
        }
        ui.model = model;
    }
    else
    {
        if (!res.GetUIModel("default_rect"))
            res.CreateUIModel("default_rect", ::UIType::Color);
        ui.model = res.GetUIModel("default_rect");
    }

    std::string shaderName = StripQuotes(node.GetChildValue("shader"));
    if (shaderName.empty())
        shaderName = StripQuotes(node.GetChildValue("Shader", "uiShader"));
    ui.shader = res.GetShader(shaderName);
    ui.shaderName = shaderName;
}

void ComponentLoader::LoadTransform(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& p = scene.registry.get<PositionComponent>(entity);
    auto& r = scene.registry.get<RotationComponent>(entity);
    auto& s = scene.registry.get<ScaleComponent>(entity);

    std::stringstream pSS(node.GetChildValue("Position", "0 0 0"));
    pSS >> p.value.x >> p.value.y >> p.value.z;
    p.prev = p.value;

    std::stringstream rSS(node.GetChildValue("Rotation", "0 0 0"));
    glm::vec3 euler;
    rSS >> euler.x >> euler.y >> euler.z;
    r.value = glm::quat(glm::radians(euler));
    r.prev = r.value;

    std::stringstream sSS(node.GetChildValue("Scale", "1 1 1"));
    sSS >> s.value.x >> s.value.y >> s.value.z;
    s.prev = s.value;

    if (scene.registry.all_of<WorldTransformComponent>(entity))
        scene.registry.get<WorldTransformComponent>(entity).isDirty = true;
}

void ComponentLoader::LoadParticleEmitter(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "SpawnRate", "Lifetime", "StartSize", "EndSize", "StartColor", "EndColor",
                               "MinVelocity", "MaxVelocity"},
                              "ParticleEmitter");
    auto& pe = scene.registry.emplace<ParticleEmitterComponent>(entity);

    pe.isActive = node.GetChildValue("Active", "true") == "true";
    pe.emitter.SpawnRate = std::stof(node.GetChildValue("SpawnRate", "10.0"));
    pe.emitter.LifeTime = std::stof(node.GetChildValue("Lifetime", "2.0"));
    pe.emitter.StartSize = std::stof(node.GetChildValue("StartSize", "0.1"));
    pe.emitter.EndSize = std::stof(node.GetChildValue("EndSize", "0.1"));

    std::string texName = node.GetChildValue("Texture");
    if (!texName.empty())
    {
        pe.textureName = texName;
        pe.emitter.Texture = res.GetTextureAuto(texName);
    }

    std::string shaderName = node.GetChildValue("Shader");
    if (!shaderName.empty())
    {
        pe.customShader = shaderName;
    }

    std::stringstream sc(node.GetChildValue("StartColor", "1 1 1 1"));
    sc >> pe.emitter.StartColor.r >> pe.emitter.StartColor.g >> pe.emitter.StartColor.b >> pe.emitter.StartColor.a;

    std::stringstream ecSS(node.GetChildValue("EndColor", "1 1 1 0"));
    ecSS >> pe.emitter.EndColor.r >> pe.emitter.EndColor.g >> pe.emitter.EndColor.b >> pe.emitter.EndColor.a;

    std::stringstream mvSS(node.GetChildValue("MinVelocity", "-0.1 1.0 -0.1"));
    mvSS >> pe.emitter.MinVelocity.x >> pe.emitter.MinVelocity.y >> pe.emitter.MinVelocity.z;

    std::stringstream xvSS(node.GetChildValue("MaxVelocity", "0.1 4.0 0.1"));
    xvSS >> pe.emitter.MaxVelocity.x >> pe.emitter.MaxVelocity.y >> pe.emitter.MaxVelocity.z;

    pe.emitter.Initialize(500);
}

void ComponentLoader::LoadUIText(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Text", "text", "Font", "font", "fontSize", "Color", "color", "Scale", "scale",
                               "Alignment", "alignment", "wordWrap", "maxWidth"},
                              "UIText");
    auto& txt = scene.registry.emplace<UITextComponent>(entity);

    auto StripQuotes = [](std::string s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.length() - 2);
        return s;
    };

    std::string textContent = node.GetChildValue("text");
    if (textContent.empty())
        textContent = node.GetChildValue("Text");
    txt.text = StripQuotes(textContent);

    std::string fontName = StripQuotes(node.GetChildValue("font"));
    if (fontName.empty())
        fontName = StripQuotes(node.GetChildValue("Font"));

    int fontSize = std::stoi(node.GetChildValue("fontSize", "60"));
    txt.fontName = fontName;
    txt.font = res.GetFontAuto(fontName, fontSize);

    std::stringstream colorSS(node.GetChildValue("color", "1 1 1 1"));
    if (node.GetChildValue("color").empty())
        colorSS.str(node.GetChildValue("Color", "1 1 1 1"));
    colorSS >> txt.color.r >> txt.color.g >> txt.color.b >> txt.color.a;

    txt.scale = std::stof(node.GetChildValue("scale", "1.0"));
    if (node.GetChildValue("scale").empty())
        txt.scale = std::stof(node.GetChildValue("Scale", "1.0"));

    std::string alignStr = node.GetChildValue("alignment", "Left");
    if (alignStr == "Center")
        txt.alignment = TextAlignment::Center;
    else if (alignStr == "Right")
        txt.alignment = TextAlignment::Right;
    else
        txt.alignment = TextAlignment::Left;

    txt.wordWrap = node.GetChildValue("wordWrap", "0") == "1" || node.GetChildValue("wordWrap", "true") == "true";
    txt.maxWidth = std::stof(node.GetChildValue("maxWidth", "0.0"));

    if (!res.GetUIModel("default_text_rect"))
        res.CreateUIModel("default_text_rect", ::UIType::Text);
    txt.model = res.GetUIModel("default_text_rect");
    txt.shader = res.GetShader("textShader");
    txt.shaderName = "textShader";
}

void ComponentLoader::LoadUIFlex(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"direction", "spacing", "autoSize", "padding"}, "UIFlex");
    auto& flex = scene.registry.emplace<UIFlexLayoutComponent>(entity);

    std::string dirStr = node.GetChildValue("direction", "Column");
    flex.direction = (dirStr == "Row") ? FlexDirection::Row : FlexDirection::Column;

    flex.spacing = std::stof(node.GetChildValue("spacing", "5.0"));

    std::stringstream padSS(node.GetChildValue("padding", "0 0 0 0"));
    padSS >> flex.padding.x >> flex.padding.y >> flex.padding.z >> flex.padding.w;
}

void ComponentLoader::LoadLOD(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Models", "Distances"}, "LOD");

    std::string modelsStr = node.GetChildValue("Models");
    std::string distancesStr = node.GetChildValue("Distances");

    if (modelsStr.empty() || distancesStr.empty())
    {
        LOGGER_WARN("ComponentLoader") << "LOD component missing 'Models' or 'Distances' field";
        return;
    }

    auto& lod = scene.registry.emplace<LODComponent>(entity);

    std::stringstream modelSS(modelsStr);
    std::string modelName;
    while (modelSS >> modelName)
    {
        auto model = res.GetModelAuto(modelName, false);

        if (!model)
        {
            LOGGER_WARN("ComponentLoader") << "LOD Model not found: " << modelName;
        }
        lod.lodModels.push_back(model);
    }

    std::stringstream distSS(distancesStr);
    float distance;
    while (distSS >> distance)
    {
        if (distance < 0)
        {
            LOGGER_WARN("ComponentLoader") << "LOD Distance cannot be negative: " << distance;
            distance = 0.0f;
        }
        lod.lodDistancesSq.push_back(distance * distance);
    }

    if (lod.lodModels.size() != lod.lodDistancesSq.size())
    {
        LOGGER_WARN("ComponentLoader") << "LOD component models count (" << lod.lodModels.size()
                                       << ") does not match distances count (" << lod.lodDistancesSq.size() << ")";
    }
}

void ComponentLoader::LoadSkyboxRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Skybox", "Shader"}, "SkyboxRenderer");

    auto& comp = scene.registry.emplace<SkyboxRenderComponent>(entity);

    std::string skyboxName = node.GetChildValue("Skybox");
    std::string shaderName = node.GetChildValue("Shader");

    comp.skybox = res.GetSkybox(skyboxName);
    if (!comp.skybox && skyboxName.find('|') != std::string::npos)
    {
    }

    comp.shader = res.GetShader(shaderName);
    comp.shaderName = shaderName;

    if (!comp.skybox)
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer skybox not found: " << skyboxName;
    if (comp.shader.expired())
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer shader not found: " << shaderName;
    EntityManager::SetActiveSkybox(scene, entity);
}

void ComponentLoader::LoadReflectionProbe(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Type", "Resolution", "BoxProjection", "BoxMin", "BoxMax", "BlendDistance"},
                              "ReflectionProbe");

    auto& comp = scene.registry.emplace<ReflectionProbeComponent>(entity);

    std::string typeStr = node.GetChildValue("Type", "Static");
    comp.type =
        (typeStr == "Dynamic" || typeStr == "DYNAMIC") ? ReflectionProbeType::Dynamic : ReflectionProbeType::Static;

    comp.resolution = std::stoi(node.GetChildValue("Resolution", "512"));
    comp.boxProjection = node.GetChildValue("BoxProjection", "true") == "true";

    if (!node.GetChildValue("BoxMin").empty())
    {
        std::stringstream ss(node.GetChildValue("BoxMin"));
        ss >> comp.boxMin.x >> comp.boxMin.y >> comp.boxMin.z;
    }
    if (!node.GetChildValue("BoxMax").empty())
    {
        std::stringstream ss(node.GetChildValue("BoxMax"));
        ss >> comp.boxMax.x >> comp.boxMax.y >> comp.boxMax.z;
    }
    comp.blendDistance = std::stof(node.GetChildValue("BlendDistance", "1.0"));
    comp.lastResolution = 0;  // Force cubemap allocation on first capture
}

void ComponentLoader::LoadAudioSource(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Audio", "Path", "Volume", "Pitch", "Pan", "Speed", "Loop", "Is3d", "MinDistance",
                               "MaxDistance", "Velocity", "PlayOnAwake"},
                              "AudioSource");

    AudioSourceComponent audio;

    std::string audioName = node.GetChildValue("Audio");
    if (audioName.empty())
        audioName = node.GetChildValue("Sound");
    if (audioName.empty())
        audioName = node.GetChildValue("Path");

    if (audioName.empty())
    {
        LOGGER_WARN("ComponentLoader") << "AudioSource missing 'Audio' property";
    }
    else
    {
        auto audioSvc = ServiceLocator::Instance().Resolve<AudioService>();
        IAudioEngine* engine = audioSvc ? audioSvc->GetEngine() : nullptr;
        audio.source = res.GetSoundAuto(audioName, engine);
        audio.resourceName = audioName;
        std::string velStr = node.GetChildValue("Velocity", "0 0 0");
        std::stringstream ss(velStr);
        ss >> audio.velocity.x >> audio.velocity.y >> audio.velocity.z;
    }

    audio.playOnAwake =
        node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";
    audio.loop = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";
    audio.is3D = node.GetChildValue("Is3d", "0") == "1" || node.GetChildValue("Is3d", "false") == "true";
    audio.volume = std::stof(node.GetChildValue("Volume", "1.0"));
    audio.pitch = std::stof(node.GetChildValue("Pitch", "1.0"));
    audio.pan = std::stof(node.GetChildValue("Pan", "0.0"));
    audio.speed = std::stof(node.GetChildValue("Speed", "1.0"));
    audio.minDistance = std::stof(node.GetChildValue("MinDistance", "1.0"));
    audio.maxDistance = std::stof(node.GetChildValue("MaxDistance", "100.0"));

    scene.registry.emplace<AudioSourceComponent>(entity, audio);
}

void ComponentLoader::LoadVideoPlayer(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Path", "Loop", "Speed", "PlayOnAwake", "Volume", "MaxDecodes"}, "VideoPlayer");

    VideoPlayerComponent video;
    video.filePath = FileSystem::getPath(node.GetChildValue("Path"));
    video.isLooping = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";

    video.speed = std::stof(node.GetChildValue("Speed", "1.0"));
    if (video.speed < 0.0f)
        LOGGER_WARN("ComponentLoader") << "VideoPlayer Speed must be positive: " << video.speed;

    video.volume = std::stof(node.GetChildValue("Volume", "1.0"));
    video.maxDecodes = std::stoi(node.GetChildValue("MaxDecodes", "1"));

    video.playOnAwake =
        node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";

    scene.registry.emplace<VideoPlayerComponent>(entity, video);
}

void ComponentLoader::LoadMaterial(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(
        node,
        {"Opacity", "Roughness", "Metallic", "Albedo", "Normal", "MetallicMap", "RoughnessMap", "AO", "EmissiveMap",
         "SpecularMap", "Emission", "AlphaCutoff", "UVScale", "UVOffset", "AO_Map"},
        "Material");
    auto& mat = scene.registry.emplace<AxisMaterialComponent>(entity);
    mat.desc.opacity = std::stof(node.GetChildValue("Opacity", "1.0"));
    mat.desc.pbr.roughness = std::stof(node.GetChildValue("Roughness", "0.5"));
    mat.desc.pbr.metallic = std::stof(node.GetChildValue("Metallic", "0.0"));
    mat.desc.pbr.ao = std::stof(node.GetChildValue("AO", "1.0"));
    mat.desc.alphaCutoff = std::stof(node.GetChildValue("AlphaCutoff", "0.5"));

    std::stringstream emSS(node.GetChildValue("Emission", "0 0 0"));
    emSS >> mat.desc.emission.x >> mat.desc.emission.y >> mat.desc.emission.z;

    std::stringstream uvS(node.GetChildValue("UVScale", "1 1"));
    uvS >> mat.desc.uvScale.x >> mat.desc.uvScale.y;

    std::stringstream uvO(node.GetChildValue("UVOffset", "0 0"));
    uvO >> mat.desc.uvOffset.x >> mat.desc.uvOffset.y;

    mat.desc.albedoPath = node.GetChildValue("Albedo");
    mat.desc.normalPath = node.GetChildValue("Normal");
    mat.desc.metallicPath = node.GetChildValue("MetallicMap");
    mat.desc.roughnessPath = node.GetChildValue("RoughnessMap");
    mat.desc.aoPath = node.GetChildValue("AO_Map", node.GetChildValue("AO_Path", ""));
    mat.desc.emissivePath = node.GetChildValue("EmissiveMap");
    mat.desc.specularPath = node.GetChildValue("SpecularMap");

    mat.gpu.albedoMap = res.GetTextureAuto(mat.desc.albedoPath) ? res.GetTextureAuto(mat.desc.albedoPath)->id : 0;
    mat.gpu.normalMap = res.GetTextureAuto(mat.desc.normalPath) ? res.GetTextureAuto(mat.desc.normalPath)->id : 0;
    mat.gpu.metallicMap = res.GetTextureAuto(mat.desc.metallicPath) ? res.GetTextureAuto(mat.desc.metallicPath)->id : 0;
    mat.gpu.roughnessMap =
        res.GetTextureAuto(mat.desc.roughnessPath) ? res.GetTextureAuto(mat.desc.roughnessPath)->id : 0;
    mat.gpu.emissiveMap = res.GetTextureAuto(mat.desc.emissivePath) ? res.GetTextureAuto(mat.desc.emissivePath)->id : 0;
    mat.gpu.aoMap = res.GetTextureAuto(mat.desc.aoPath) ? res.GetTextureAuto(mat.desc.aoPath)->id : 0;
    mat.gpu.dirty = false;
}

void ComponentLoader::LoadFragment(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& frag = scene.registry.emplace<FragmentComponent>(entity);
    frag.path = node.GetChildValue("Path");

    // Check for both "Override" (singular) and "Overrides" (plural)
    const YAMLNode* overrideNode = node.GetChild("Override");
    if (!overrideNode)
        overrideNode = node.GetChild("Overrides");

    if (overrideNode)
    {
        frag.overrides = SerializeYAML(*overrideNode);
        LOGGER_INFO("ComponentLoader") << "[FRAG-LOAD] Entity " << (uint32_t)entity << " path='" << frag.path
                                       << "' overrides='" << frag.overrides << "'";
    }
    else
    {
        LOGGER_INFO("ComponentLoader") << "[FRAG-LOAD] Entity " << (uint32_t)entity << " path='" << frag.path
                                       << "' NO OVERRIDES (Override node not found). Node children:";
        for (auto& c : node.children)
        {
            LOGGER_INFO("ComponentLoader") << "  child key='" << c.key << "' value='" << c.value << "'";
        }
    }
}

void ComponentLoader::LoadPathFollower(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& pf = scene.registry.emplace<PathFollowerComponent>(entity);
    pf.moveSpeed = std::stof(node.GetChildValue("MoveSpeed", "5.0"));
    pf.arrivalDistance = std::stof(node.GetChildValue("ArrivalDistance", "0.5"));
}

void ComponentLoader::LoadDecal(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    auto& d = scene.registry.emplace<DecalComponent>(entity);
    d.opacity = std::stof(node.GetChildValue("Opacity", "1.0"));
    d.roughness = std::stof(node.GetChildValue("Roughness", "1.0"));
    d.metallic = std::stof(node.GetChildValue("Metallic", "0.0"));
    d.reflectivity = std::stof(node.GetChildValue("Reflectivity", "0.0"));

    std::stringstream ss(node.GetChildValue("TintColor", "1 1 1 1"));
    ss >> d.tintColor.r >> d.tintColor.g >> d.tintColor.b >> d.tintColor.a;
    d.customShader = node.GetChildValue("Shader");
}

void ComponentLoader::LoadPlanarReflection(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& pr = scene.registry.emplace<PlanarReflectionComponent>(entity);
    pr.resolution = std::stoi(node.GetChildValue("Resolution", "1024"));
    std::stringstream ss(node.GetChildValue("Normal", "0 1 0"));
    ss >> pr.normal.x >> pr.normal.y >> pr.normal.z;
}

void ComponentLoader::LoadLightProbe(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& lp = scene.registry.emplace<LightProbeComponent>(entity);
    lp.intensity = std::stof(node.GetChildValue("Intensity", "1.0"));
    lp.radius = std::stof(node.GetChildValue("Radius", "5.0"));
}

void ComponentLoader::LoadTerrain(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    auto& t = scene.registry.emplace<TerrainComponent>(entity);
    t.heightMapName = node.GetChildValue("HeightMap");
    std::stringstream ss(node.GetChildValue("Size", "512 50 512"));
    ss >> t.terrainSize.x >> t.terrainSize.y >> t.terrainSize.z;
    t.resolution = std::stoi(node.GetChildValue("Resolution", "1024"));
    t.textureScale = std::stof(node.GetChildValue("TextureScale", "1.0"));
    t.castShadows = node.GetChildValue("CastShadows", "true") == "true";
    t.customShader = node.GetChildValue("Shader");
}

void ComponentLoader::LoadNetwork(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& net = scene.registry.emplace<NetworkComponent>(entity);
    net.networkId = std::stoul(node.GetChildValue("NetworkId", "0"));
    net.ownerId = std::stoul(node.GetChildValue("OwnerId", "0"));
    net.isLocal = node.GetChildValue("IsLocal", "false") == "true";
}
