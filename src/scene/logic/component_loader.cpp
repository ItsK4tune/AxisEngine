#include <scene/logic/component_loader.h>
#include <scene/logic/scene.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
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
#include <ecs/logic/scriptable_system.h>
#include <navigation/unit/pathfollower_component.h>
#include <navigation/unit/navmesh_component.h>
#include <physics/logic/physics_loader.h>
#include <resource/unit/animator.h>
#include <resource/logic/resource_manager.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> ComponentLoader::s_Factories;
std::unordered_map<std::string, std::shared_ptr<IComponentSerializerFactory>> ComponentLoader::s_Serializers;
std::shared_mutex ComponentLoader::s_RegistryMutex;

namespace
{
thread_local bool s_RegisteringDefaultLoaders = false;

class DefaultLoaderRegistrationScope
{
public:
    DefaultLoaderRegistrationScope()
    {
        s_RegisteringDefaultLoaders = true;
    }
    ~DefaultLoaderRegistrationScope()
    {
        s_RegisteringDefaultLoaders = false;
    }
};

class FunctionComponentLoaderFactory final : public IComponentLoaderFactory
{
public:
    explicit FunctionComponentLoaderFactory(ComponentLoaderFunc func) : m_Func(std::move(func))
    {
    }

    void Load(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res,
              IPhysicsWorld* phys) override
    {
        m_Func(scene, entity, node, res, phys);
    }

private:
    ComponentLoaderFunc m_Func;
};

class FunctionComponentSerializerFactory final : public IComponentSerializerFactory
{
public:
    explicit FunctionComponentSerializerFactory(ComponentSerializerFunc func) : m_Func(std::move(func))
    {
    }

    bool Serialize(const entt::registry& registry, entt::entity entity, YAMLNode& component) const override
    {
        return m_Func(registry, entity, component);
    }

private:
    ComponentSerializerFunc m_Func;
};

glm::vec3 ParseVec3Value(const std::string& value, const glm::vec3& fallback = glm::vec3(0.0f))
{
    std::stringstream stream(value);
    glm::vec3 parsed = fallback;
    if (!(stream >> parsed.x >> parsed.y >> parsed.z))
        return fallback;
    return parsed;
}

bool ParseBoolValue(const std::string& value, bool fallback = false)
{
    if (value == "true" || value == "1")
        return true;
    if (value == "false" || value == "0")
        return false;
    return fallback;
}
}  // namespace

void ComponentLoader::RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory)
{
    if (type.empty())
        throw std::invalid_argument("Component loader type cannot be empty");
    if (!factory)
        throw std::invalid_argument("Component loader factory cannot be null");
    std::unique_lock lock(s_RegistryMutex);
    if (s_RegisteringDefaultLoaders)
        s_Factories.try_emplace(type, std::move(factory));
    else
        s_Factories[type] = std::move(factory);
}

void ComponentLoader::RegisterLoader(const std::string& type, ComponentLoaderFunc func)
{
    if (!func)
        throw std::invalid_argument("Component loader callback cannot be empty");
    RegisterLoader(type, std::make_shared<FunctionComponentLoaderFactory>(std::move(func)));
}

bool ComponentLoader::UnregisterLoader(const std::string& type)
{
    std::unique_lock lock(s_RegistryMutex);
    return s_Factories.erase(type) != 0;
}

void ComponentLoader::RegisterSerializer(const std::string& type, std::shared_ptr<IComponentSerializerFactory> factory)
{
    if (type.empty())
        throw std::invalid_argument("Component serializer type cannot be empty");
    if (!factory)
        throw std::invalid_argument("Component serializer factory cannot be null");
    std::unique_lock lock(s_RegistryMutex);
    s_Serializers[type] = std::move(factory);
}

void ComponentLoader::RegisterSerializer(const std::string& type, ComponentSerializerFunc func)
{
    if (!func)
        throw std::invalid_argument("Component serializer callback cannot be empty");
    RegisterSerializer(type, std::make_shared<FunctionComponentSerializerFactory>(std::move(func)));
}

bool ComponentLoader::UnregisterSerializer(const std::string& type)
{
    std::unique_lock lock(s_RegistryMutex);
    return s_Serializers.erase(type) != 0;
}

std::vector<std::pair<std::string, YAMLNode>> ComponentLoader::CollectSerializedComponents(
    const entt::registry& registry, entt::entity entity)
{
    std::vector<std::pair<std::string, std::shared_ptr<IComponentSerializerFactory>>> serializers;
    {
        std::shared_lock lock(s_RegistryMutex);
        serializers.reserve(s_Serializers.size());
        for (const auto& [type, serializer] : s_Serializers) serializers.emplace_back(type, serializer);
    }

    std::vector<std::pair<std::string, YAMLNode>> serialized;
    serialized.reserve(serializers.size());
    for (const auto& [type, serializer] : serializers)
    {
        YAMLNode component;
        component.key = "Component";
        component.value = type;
        if (serializer->Serialize(registry, entity, component))
            serialized.emplace_back(type, std::move(component));
    }
    std::sort(serialized.begin(), serialized.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    return serialized;
}

bool ComponentLoader::Load(const std::string& type, Scene& scene, entt::entity entity, const YAMLNode& node,
                           ResourceManager& res, IPhysicsWorld* phys)
{
    std::shared_ptr<IComponentLoaderFactory> factory;
    {
        std::shared_lock lock(s_RegistryMutex);
        if (auto it = s_Factories.find(type); it != s_Factories.end())
            factory = it->second;
    }
    if (!factory)
        return false;
    factory->Load(scene, entity, node, res, phys);
    return true;
}

void ComponentLoader::InitializeDefaultLoaders()
{
    static std::once_flag initialized;
    std::call_once(initialized, []() {
        // Defaults use insert-if-absent while user registration always replaces,
        // so module overrides win even if registration races first scene load.
        DefaultLoaderRegistrationScope registrationScope;

        RegisterLoader("Renderer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { LoadRenderer(s, e, n, r); });
        RegisterLoader("Animator", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { LoadAnimator(s, e, n, r); });
        RegisterLoader("Camera", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
            LoadCamera(s, e, n);
        });

        // Standardized Lighting Names
        RegisterLoader("DirectionalLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                              IPhysicsWorld* p) { LoadLightDir(s, e, n); });
        RegisterLoader("PointLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadLightPoint(s, e, n); });
        RegisterLoader("SpotLight", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadLightSpot(s, e, n); });

        // Legacy Lighting Names (Backup)
        RegisterLoader("LightDir", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { LoadLightDir(s, e, n); });
        RegisterLoader("LightPoint", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadLightPoint(s, e, n); });
        RegisterLoader("LightSpot", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadLightSpot(s, e, n); });

        RegisterLoader("UITransform", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                         IPhysicsWorld* p) { LoadUITransform(s, e, n); });
        RegisterLoader("UIRenderer", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadUIRenderer(s, e, n, r); });
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
        RegisterLoader("Animation", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadAnimator(s, e, n, r); });
        RegisterLoader("ParticleEmitter", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                             IPhysicsWorld* p) { LoadParticleEmitter(s, e, n, r); });
        RegisterLoader("Material", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { LoadMaterial(s, e, n, r); });
        RegisterLoader("LOD", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
            LoadLOD(s, e, n, r);
        });

        // New Modular Physics Components
        RegisterLoader("RigidShape", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { PhysicsLoader::LoadRigidShape(s, e, n, p); });
        RegisterLoader("RigidBody", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { PhysicsLoader::LoadRigidBody(s, e, n, p); });

        RegisterLoader("CharacterController",
                       [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
                           PhysicsLoader::LoadCharacterController(s, e, n, p);
                       });
        RegisterLoader("Transform", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadTransform(s, e, n); });
        RegisterLoader("PathFollower", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                           IPhysicsWorld* p) { ComponentLoader::LoadPathFollower(s, e, n); });
        RegisterLoader("NavMesh", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager&, IPhysicsWorld*) {
            ComponentLoader::LoadNavMesh(s, e, n);
        });
        RegisterLoader("NavigationGrid",
                       [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager&, IPhysicsWorld*) {
                           ComponentLoader::LoadNavigationGrid(s, e, n);
                       });
        RegisterLoader("Decal", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
            LoadDecal(s, e, n, r);
        });
        RegisterLoader("UIFlex", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
            LoadUIFlex(s, e, n);
        });
        RegisterLoader("UIInteractive", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                           IPhysicsWorld* p) { LoadUIInteractive(s, e, n); });
        RegisterLoader("UIAnimation", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                         IPhysicsWorld* p) { LoadUIAnimation(s, e, n); });
        RegisterLoader("Reflective", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadReflective(s, e, n, r); });
        RegisterLoader("AudioSource", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                         IPhysicsWorld* p) { LoadAudioSource(s, e, n, r); });
        RegisterLoader("Audio", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r, IPhysicsWorld* p) {
            LoadAudioSource(s, e, n, r);
        });
        RegisterLoader("Fragment", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                      IPhysicsWorld* p) { LoadFragment(s, e, n); });
        RegisterLoader("PlanarReflection", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                              IPhysicsWorld* p) { LoadPlanarReflection(s, e, n); });
        RegisterLoader("LightProbe", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                        IPhysicsWorld* p) { LoadLightProbe(s, e, n); });
        RegisterLoader("Terrain", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadTerrain(s, e, n, r); });
        RegisterLoader("Network", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                     IPhysicsWorld* p) { LoadNetwork(s, e, n); });
        RegisterLoader("Occlusion", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadOcclusion(s, e, n); });
        RegisterLoader("Streaming", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                       IPhysicsWorld* p) { LoadStreaming(s, e, n); });
        RegisterLoader("Script", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                    IPhysicsWorld* p) { LoadScript(s, e, n); });
    });
}

void ComponentLoader::LoadScript(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    ScriptableSystem::LoadScript(scene, entity, node);
}

bool ComponentLoader::HasSerializedComponents(const entt::registry& registry, entt::entity entity)
{
    std::vector<std::shared_ptr<IComponentSerializerFactory>> serializers;
    {
        std::shared_lock lock(s_RegistryMutex);
        serializers.reserve(s_Serializers.size());
        for (const auto& [type, serializer] : s_Serializers) serializers.push_back(serializer);
    }

    for (const auto& serializer : serializers)
    {
        YAMLNode component;
        component.key = "Component";
        if (serializer->Serialize(registry, entity, component))
            return true;
    }
    return false;
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

void ComponentLoader::LoadRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(
        node, {"Model", "Shader", "Order", "Color", "CastShadow", "ReceiveShadow", "IgnoreDepth", "RenderMode"},
        "Renderer");

    std::string modelName = node.GetChildValue("Model");
    std::string shaderName = node.GetChildValue("Shader");
    int order = LoaderUtils::SafeStoi(node.GetChildValue("Order", "0"));
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
    if (scene.HasAllComponents<InfoComponent>(entity))
        entityName = scene.GetComponent<InfoComponent>(entity).name;

    auto& r = scene.AddComponent<MeshRendererComponent>(entity);
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
    r.modelName = modelName;
    r.shader = res.GetShader(shaderName);
    r.shaderName = shaderName;

    r.order = order;
    r.castShadow = castShadow;
    r.receiveShadow = receiveShadow;
    r.ignoreDepth = ignoreDepth;
    r.color = color;
    int renderModeValue = LoaderUtils::SafeStoi(node.GetChildValue("RenderMode", "0"));
    r.renderMode = (renderModeValue == (int)RenderMode::ForceForward) ? RenderMode::ForceForward : RenderMode::Auto;

    if (!r.model)
        LOGGER_WARN("ComponentLoader") << "Renderer model not found on entity '" << entityName << "': " << modelName;
    if (r.shader.expired())
        LOGGER_WARN("ComponentLoader") << "Renderer shader not found on entity '" << entityName << "': " << shaderName;
}

void ComponentLoader::LoadAnimator(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Animation", "Speed", "StartTime", "Rate", "BlendFactor", "GraphEnabled",
                               "GraphEntry", "GraphParameter", "GraphState", "GraphTransition",
                               "GraphTransitionV2"},
                              "Animator");

    auto& a = scene.AddComponent<AnimationComponent>(entity);

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

    a.speed = LoaderUtils::SafeStof(node.GetChildValue("Speed", "1.0"));
    a.startTime = LoaderUtils::SafeStof(node.GetChildValue("StartTime", "0.0"));
    a.rate = LoaderUtils::SafeStof(node.GetChildValue("Rate", "30.0"));
    a.blendFactor = LoaderUtils::SafeStof(node.GetChildValue("BlendFactor", "0.0"));
    a.graph.enabled = node.GetChildValue("GraphEnabled", "false") == "true";
    a.graph.entryState = static_cast<uint32_t>(LoaderUtils::SafeStoul(node.GetChildValue("GraphEntry", "0")));

    for (const auto& child : node.children)
    {
        std::istringstream record(child.value);
        if (child.key == "GraphParameter")
        {
            AnimationGraphParameter parameter;
            int type = 0;
            record >> std::quoted(parameter.name) >> type >> parameter.floatValue >> parameter.boolValue >>
                parameter.triggerValue;
            parameter.type = static_cast<AnimationParameterType>(type);
            if (!parameter.name.empty())
                a.graph.parameters.push_back(std::move(parameter));
        }
        else if (child.key == "GraphState")
        {
            AnimationGraphState state;
            record >> state.id >> std::quoted(state.name) >> std::quoted(state.clip) >> state.speed >>
                state.editorPosition.x >> state.editorPosition.y;
            if (state.id != 0)
            {
                a.graph.nextId = std::max(a.graph.nextId, state.id + 1);
                a.graph.states.push_back(std::move(state));
            }
        }
        else if (child.key == "GraphTransition" || child.key == "GraphTransitionV2")
        {
            AnimationGraphTransition transition;
            size_t conditionCount = 0;
            record >> transition.id >> transition.fromState >> transition.toState >> transition.duration >>
                transition.hasExitTime >> transition.exitTime;
            if (child.key == "GraphTransitionV2")
            {
                int logic = 0;
                record >> logic >> conditionCount;
                transition.conditionLogic = static_cast<GraphConditionLogic>(logic);
            }
            else
            {
                record >> conditionCount;
            }
            for (size_t index = 0; index < conditionCount; ++index)
            {
                AnimationGraphCondition condition;
                int op = 0;
                record >> std::quoted(condition.parameter) >> op >> condition.threshold;
                if (child.key == "GraphTransitionV2")
                    record >> condition.negated;
                condition.op = static_cast<AnimationConditionOp>(op);
                transition.conditions.push_back(std::move(condition));
            }
            if (transition.id != 0)
            {
                a.graph.nextId = std::max(a.graph.nextId, transition.id + 1);
                a.graph.transitions.push_back(std::move(transition));
            }
        }
    }

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
            a.animator->SetTime(a.startTime);
            a.animator->SetUpdateRate(a.rate);
            a.animator->SetBlendFactor(a.blendFactor);

            if (scene.HasAllComponents<MeshRendererComponent>(entity))
            {
                auto& mrc = scene.GetComponent<MeshRendererComponent>(entity);
                if (mrc.model && mrc.model->IsStatic())
                {
                    LOGGER_WARN("ComponentLoader")
                        << "Entity has Animator but its Model is STATIC! Animations will not play correctly. Entity: "
                        << (scene.HasAllComponents<InfoComponent>(entity)
                                ? scene.GetComponent<InfoComponent>(entity).name
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
    auto& pp = scene.AddComponent<PostProcessComponent>(entity);
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
                effect.priority = LoaderUtils::SafeStoi(parts[1]);
            if (parts.size() >= 6)
            {
                effect.x = LoaderUtils::SafeStoi(parts[2]);
                effect.y = LoaderUtils::SafeStoi(parts[3]);
                effect.w = LoaderUtils::SafeStoi(parts[4]);
                effect.h = LoaderUtils::SafeStoi(parts[5]);
            }
            if (parts.size() >= 7)
            {
                effect.affectUI = (parts[6] == "1");
            }
            if (parts.size() >= 8)
            {
                const auto knownInputs = static_cast<unsigned long>(PostProcessInput::All);
                const auto serializedInputs =
                    LoaderUtils::SafeStoul(parts[7], static_cast<unsigned long>(PostProcessInput::Color));
                effect.inputs = static_cast<PostProcessInput>(serializedInputs & knownInputs);
            }
            if (parts.size() >= 9)
                effect.enabled = parts[8] == "1" || parts[8] == "true";
            pp.effects.push_back(effect);
        }
    }
}

void ComponentLoader::LoadReflective(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Active", "Reflectivity", "FresnelPower", "FresnelBias", "Probe"}, "Reflective");
    auto& ref = scene.AddComponent<ReflectiveComponent>(entity);
    ref.enabled = node.GetChildValue("Active", "true") == "true";
    ref.reflectivity = LoaderUtils::SafeStof(node.GetChildValue("Reflectivity", "1.0"));
    ref.fresnelPower = LoaderUtils::SafeStof(node.GetChildValue("FresnelPower", "5.0"));
    ref.fresnelBias = LoaderUtils::SafeStof(node.GetChildValue("FresnelBias", "0.04"));
    ref.targetProbe = node.GetChildValue("Probe", "");
}

void ComponentLoader::LoadCamera(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Primary", "FOV", "Yaw", "Pitch", "Near", "Far", "AspectRatio", "ScreenWidth",
                               "ScreenHeight", "Orthographic", "OrthoSize", "CullingMask"},
                              "Camera");

    auto& c = scene.AddComponent<CameraComponent>(entity);
    c.isPrimary = node.GetChildValue("Primary", "1") == "1" || node.GetChildValue("Primary", "true") == "true";

    c.aspectRatio = LoaderUtils::SafeStof(node.GetChildValue("AspectRatio", "0.0"));
    c.screenWidth = std::max(1, LoaderUtils::SafeStoi(node.GetChildValue("ScreenWidth", "800")));
    c.screenHeight = std::max(1, LoaderUtils::SafeStoi(node.GetChildValue("ScreenHeight", "600")));
    c.isOrthographic =
        node.GetChildValue("Orthographic", "false") == "true" || node.GetChildValue("Orthographic", "0") == "1";
    c.orthoSize = LoaderUtils::SafeStof(node.GetChildValue("OrthoSize", "5.0"));
    c.cullingMask = static_cast<uint32_t>(LoaderUtils::SafeStoul(node.GetChildValue("CullingMask", "4294967295")));

    c.fov = LoaderUtils::SafeStof(node.GetChildValue("FOV", "45.0"));
    if (c.fov <= 0.0f || c.fov >= 180.0f)
        LOGGER_WARN("ComponentLoader") << "Camera FOV out of bounds (0-180): " << c.fov;

    if (node.GetChild("Yaw") || node.GetChild("Pitch"))
    {
        float yaw = LoaderUtils::SafeStof(node.GetChildValue("Yaw", "-90.0"));
        float pitch = LoaderUtils::SafeStof(node.GetChildValue("Pitch", "0.0"));
        if (pitch < -89.0f || pitch > 89.0f)
            LOGGER_WARN("ComponentLoader") << "Camera Pitch out of bounds (-89 to 89): " << pitch;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const glm::vec3 right = glm::normalize(glm::cross(glm::normalize(front), worldUp));
        const glm::vec3 up = glm::normalize(glm::cross(right, glm::normalize(front)));
        const glm::quat rotation = glm::quatLookAt(glm::normalize(front), up);

        auto& rotComp = scene.GetOrAddComponent<RotationComponent>(entity);
        rotComp.value = rotation;
        rotComp.prev = rotation;
    }

    c.nearPlane = LoaderUtils::SafeStof(node.GetChildValue("Near", "0.1"));
    c.farPlane = LoaderUtils::SafeStof(node.GetChildValue("Far", "1000.0"));

    if (c.nearPlane <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane should be > 0";
    if (c.nearPlane >= c.farPlane)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane must be less than Far plane";

    if (c.isPrimary)
        scene.SetActiveCamera(entity);
}

void ComponentLoader::LoadLightDir(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(
        node, {"Active", "CastShadow", "Direction", "Color", "Intensity", "Ambient", "Diffuse", "Specular"},
        "LightDir");

    auto& l = scene.AddComponent<DirectionalLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream directionSS(node.GetChildValue("Direction", "-0.2 -1 -0.3"));
    directionSS >> l.direction.x >> l.direction.y >> l.direction.z;
    if (glm::length(l.direction) > 0.0001f)
        l.direction = glm::normalize(l.direction);

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = LoaderUtils::SafeStof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightDir Intensity should not be negative: " << l.intensity;

    l.ambient = LoaderUtils::SafeStof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = LoaderUtils::SafeStof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = LoaderUtils::SafeStof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadLightPoint(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "CastShadow", "Color", "Intensity", "Radius", "Constant", "Linear",
                               "Quadratic", "Ambient", "Diffuse", "Specular"},
                              "LightPoint");

    auto& l = scene.AddComponent<PointLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = LoaderUtils::SafeStof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightPoint Intensity should not be negative: " << l.intensity;

    l.radius = LoaderUtils::SafeStof(node.GetChildValue("Radius", "10.0"));
    if (l.radius <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightPoint Radius should be positive: " << l.radius;

    l.constant = LoaderUtils::SafeStof(node.GetChildValue("Constant", "1.0"));
    l.linear = LoaderUtils::SafeStof(node.GetChildValue("Linear", "0.09"));
    l.quadratic = LoaderUtils::SafeStof(node.GetChildValue("Quadratic", "0.032"));

    l.ambient = LoaderUtils::SafeStof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = LoaderUtils::SafeStof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = LoaderUtils::SafeStof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadLightSpot(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "CastShadow", "Direction", "Color", "Intensity", "CutOff", "OuterCutOff",
                               "Constant", "Linear", "Quadratic", "Radius", "Ambient", "Diffuse", "Specular"},
                              "LightSpot");

    auto& l = scene.AddComponent<SpotLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream directionSS(node.GetChildValue("Direction", "0 -1 0"));
    directionSS >> l.direction.x >> l.direction.y >> l.direction.z;
    if (glm::length(l.direction) > 0.0001f)
        l.direction = glm::normalize(l.direction);

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r = 1, g = 1, b = 1;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = LoaderUtils::SafeStof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot Intensity should not be negative: " << l.intensity;
    l.radius = LoaderUtils::SafeStof(node.GetChildValue("Radius", "50.0"));
    if (l.radius <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot Radius should be positive: " << l.radius;

    float cutOffAng = LoaderUtils::SafeStof(node.GetChildValue("CutOff", "12.5"));
    float outerCutOffAng = LoaderUtils::SafeStof(node.GetChildValue("OuterCutOff", "17.5"));

    if (cutOffAng < 0.0f || cutOffAng > 90.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot CutOff out of bounds (0-90): " << cutOffAng;
    if (outerCutOffAng < 0.0f || outerCutOffAng > 90.0f)
        LOGGER_WARN("ComponentLoader") << "LightSpot OuterCutOff out of bounds (0-90): " << outerCutOffAng;
    if (cutOffAng > outerCutOffAng)
        LOGGER_WARN("ComponentLoader") << "LightSpot CutOff should be less than or equal to OuterCutOff";

    l.cutOff = glm::cos(glm::radians(cutOffAng));
    l.outerCutOff = glm::cos(glm::radians(outerCutOffAng));

    l.constant = LoaderUtils::SafeStof(node.GetChildValue("Constant", "1.0"));
    l.linear = LoaderUtils::SafeStof(node.GetChildValue("Linear", "0.09"));
    l.quadratic = LoaderUtils::SafeStof(node.GetChildValue("Quadratic", "0.032"));

    l.ambient = LoaderUtils::SafeStof(node.GetChildValue("Ambient", "0.1"));
    l.diffuse = LoaderUtils::SafeStof(node.GetChildValue("Diffuse", "0.8"));
    l.specular = LoaderUtils::SafeStof(node.GetChildValue("Specular", "0.5"));
}

void ComponentLoader::LoadUITransform(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Position", "Size", "ZOrder", "zIndex", "Rotation", "rotation", "anchorMin", "anchorMax",
                               "offsetMin", "offsetMax", "pivot", "flipX", "FlipX", "flipY", "FlipY"},
                              "UITransform");
    auto& ui = scene.AddComponent<UITransformComponent>(entity);

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
            v = LoaderUtils::SafeStof(t, v);
        };

        parseComp(xStr, outVec.x, outPercent.x);
        parseComp(yStr, outVec.y, outPercent.y);
    };

    parseVec2Percent(node.GetChildValue("Position"), ui.position, ui.positionIsPercent, glm::vec2(0.0f));
    parseVec2Percent(node.GetChildValue("Size"), ui.size, ui.sizeIsPercent, glm::vec2(100.0f));

    auto parseBool = [](const std::string& value, bool defaultValue = false) {
        if (value.empty())
            return defaultValue;
        return value == "true" || value == "True" || value == "TRUE" || value == "1" || value == "yes" ||
               value == "Yes" || value == "on" || value == "On";
    };

    ui.zIndex = LoaderUtils::SafeStoi(node.GetChildValue("ZOrder", "0"));
    std::string zLabel = node.GetChildValue("zIndex");
    if (!zLabel.empty())
        ui.zIndex = LoaderUtils::SafeStoi(zLabel);
    ui.rotation = LoaderUtils::SafeStof(node.GetChildValue("rotation", node.GetChildValue("Rotation", "0")));

    parseVec2Percent(node.GetChildValue("anchorMin"), ui.anchorMin, ui.anchorMinIsPercent, glm::vec2(0.5f));
    parseVec2Percent(node.GetChildValue("anchorMax"), ui.anchorMax, ui.anchorMaxIsPercent, glm::vec2(0.5f));
    parseVec2Percent(node.GetChildValue("offsetMin"), ui.offsetMin, ui.offsetMinIsPercent, glm::vec2(-50.0f));
    parseVec2Percent(node.GetChildValue("offsetMax"), ui.offsetMax, ui.offsetMaxIsPercent, glm::vec2(50.0f));

    std::stringstream pivotSS(node.GetChildValue("pivot", "0.5 0.5"));
    pivotSS >> ui.pivot.x >> ui.pivot.y;

    std::string flipX = node.GetChildValue("flipX");
    if (flipX.empty())
        flipX = node.GetChildValue("FlipX");
    std::string flipY = node.GetChildValue("flipY");
    if (flipY.empty())
        flipY = node.GetChildValue("FlipY");
    ui.flipX = parseBool(flipX);
    ui.flipY = parseBool(flipY);
}

void ComponentLoader::LoadUIRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Color", "color", "Texture", "texture", "Shader", "shader"}, "UIRenderer");
    auto& ui = scene.AddComponent<UIRendererComponent>(entity);

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
        ui.textureName = textureName;
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
    auto& p = scene.GetComponent<PositionComponent>(entity);
    auto& r = scene.GetComponent<RotationComponent>(entity);
    auto& s = scene.GetComponent<ScaleComponent>(entity);

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

    scene.MarkTransformDirty(entity);
}

void ComponentLoader::LoadParticleEmitter(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Active", "SpawnRate", "Lifetime", "Duration", "StartSize", "EndSize", "StartColor",
                               "EndColor", "MinVelocity", "MaxVelocity", "Texture", "Shader", "MaxParticles",
                               "Shape", "Gravity", "Drag", "GraphEnabled", "GraphParameter", "GraphNode",
                               "GraphLink", "GraphLinkV2"},
                              "ParticleEmitter");
    auto& pe = scene.AddComponent<ParticleEmitterComponent>(entity);

    pe.isActive = node.GetChildValue("Active", "true") == "true";
    pe.emitter.SpawnRate = LoaderUtils::SafeStof(node.GetChildValue("SpawnRate", "10.0"));
    pe.emitter.LifeTime = LoaderUtils::SafeStof(node.GetChildValue("Lifetime", "2.0"));
    std::string shape = node.GetChildValue("Shape", "DIRECTIONAL");
    std::transform(shape.begin(), shape.end(), shape.begin(),
                   [](unsigned char value) { return static_cast<char>(std::toupper(value)); });
    if (shape == "CONE")
        pe.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
    else if (shape == "FIGURE_EIGHT" || shape == "FIGUREEIGHT")
        pe.emitter.Shape = ParticleEmitter::EmissionShape::FIGURE_EIGHT;
    else
        pe.emitter.Shape = ParticleEmitter::EmissionShape::DIRECTIONAL;
    pe.emissionDuration = LoaderUtils::SafeStof(node.GetChildValue("Duration", "-1.0"));
    pe.emitter.StartSize = LoaderUtils::SafeStof(node.GetChildValue("StartSize", "0.1"));
    pe.emitter.EndSize = LoaderUtils::SafeStof(node.GetChildValue("EndSize", "0.1"));
    pe.emitter.Drag = std::max(0.0f, LoaderUtils::SafeStof(node.GetChildValue("Drag", "0.0")));
    std::stringstream gravity(node.GetChildValue("Gravity", "0 0 0"));
    gravity >> pe.emitter.Gravity.x >> pe.emitter.Gravity.y >> pe.emitter.Gravity.z;

    pe.graph.enabled = node.GetChildValue("GraphEnabled", "false") == "true";
    for (const auto& child : node.children)
    {
        std::istringstream record(child.value);
        if (child.key == "GraphParameter")
        {
            AnimationGraphParameter parameter;
            int type = 0;
            record >> std::quoted(parameter.name) >> type >> parameter.floatValue >> parameter.boolValue >>
                parameter.triggerValue;
            parameter.type = static_cast<AnimationParameterType>(type);
            if (!parameter.name.empty())
                pe.graph.parameters.push_back(std::move(parameter));
        }
        else if (child.key == "GraphNode")
        {
            VFXGraphNode graphNode;
            int type = 0;
            record >> graphNode.id >> type >> std::quoted(graphNode.name) >> graphNode.enabled >> graphNode.scalarA >>
                graphNode.scalarB >> graphNode.valueA.x >> graphNode.valueA.y >> graphNode.valueA.z >>
                graphNode.valueA.w >> graphNode.valueB.x >> graphNode.valueB.y >> graphNode.valueB.z >>
                graphNode.valueB.w >> graphNode.editorPosition.x >> graphNode.editorPosition.y;
            graphNode.type = static_cast<VFXNodeType>(type);
            if (graphNode.id != 0)
            {
                pe.graph.nextId = std::max(pe.graph.nextId, graphNode.id + 1);
                pe.graph.nodes.push_back(std::move(graphNode));
            }
        }
        else if (child.key == "GraphLink" || child.key == "GraphLinkV2")
        {
            VFXGraphLink link;
            record >> link.id >> link.fromNode >> link.toNode;
            if (child.key == "GraphLinkV2")
            {
                int logic = 0;
                size_t conditionCount = 0;
                record >> logic >> conditionCount;
                link.conditionLogic = static_cast<GraphConditionLogic>(logic);
                for (size_t index = 0; index < conditionCount; ++index)
                {
                    AnimationGraphCondition condition;
                    int op = 0;
                    record >> std::quoted(condition.parameter) >> op >> condition.threshold >> condition.negated;
                    condition.op = static_cast<AnimationConditionOp>(op);
                    link.conditions.push_back(std::move(condition));
                }
            }
            if (link.id != 0)
            {
                pe.graph.nextId = std::max(pe.graph.nextId, link.id + 1);
                pe.graph.links.push_back(link);
            }
        }
    }

    std::string texName = node.GetChildValue("Texture");
    if (!texName.empty())
    {
        pe.textureName = texName;
        pe.emitter.texture = res.GetTextureAuto(texName);
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

    pe.maxParticles =
        static_cast<unsigned int>(std::max(1, LoaderUtils::SafeStoi(node.GetChildValue("MaxParticles", "500"))));
    pe.emitter.Initialize(pe.maxParticles);
}

void ComponentLoader::LoadUIText(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Text", "text", "Font", "font", "fontSize", "Color", "color", "Scale", "scale",
                               "Alignment", "alignment", "wordWrap", "maxWidth", "wrapByWord", "Shader", "shader"},
                              "UIText");
    auto& txt = scene.AddComponent<UITextComponent>(entity);

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

    int fontSize = LoaderUtils::SafeStoi(node.GetChildValue("fontSize", "60"));
    txt.fontSize = fontSize;
    txt.fontName = fontName;
    txt.font = res.GetFontAuto(fontName, fontSize);

    std::stringstream colorSS(node.GetChildValue("color", "1 1 1 1"));
    if (node.GetChildValue("color").empty())
        colorSS.str(node.GetChildValue("Color", "1 1 1 1"));
    colorSS >> txt.color.r >> txt.color.g >> txt.color.b >> txt.color.a;

    txt.scale = LoaderUtils::SafeStof(node.GetChildValue("scale", "1.0"));
    if (node.GetChildValue("scale").empty())
        txt.scale = LoaderUtils::SafeStof(node.GetChildValue("Scale", "1.0"));

    std::string alignStr = node.GetChildValue("alignment", "Left");
    if (alignStr == "Center")
        txt.alignment = TextAlignment::Center;
    else if (alignStr == "Right")
        txt.alignment = TextAlignment::Right;
    else
        txt.alignment = TextAlignment::Left;

    txt.wordWrap = node.GetChildValue("wordWrap", "0") == "1" || node.GetChildValue("wordWrap", "true") == "true";
    txt.maxWidth = LoaderUtils::SafeStof(node.GetChildValue("maxWidth", "0.0"));
    txt.wrapByWord = node.GetChildValue("wrapByWord", "true") == "true" || node.GetChildValue("wrapByWord", "1") == "1";

    if (!res.GetUIModel("default_text_rect"))
        res.CreateUIModel("default_text_rect", ::UIType::Text);
    txt.model = res.GetUIModel("default_text_rect");
    txt.shaderName = node.GetChildValue("shader", node.GetChildValue("Shader", "textShader"));
    txt.shader = res.GetShader(txt.shaderName);
}

void ComponentLoader::LoadUIFlex(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"direction", "spacing", "autoSize", "padding"}, "UIFlex");
    auto& flex = scene.AddComponent<UIFlexLayoutComponent>(entity);

    std::string dirStr = node.GetChildValue("direction", "Column");
    flex.direction = (dirStr == "Row") ? FlexDirection::Row : FlexDirection::Column;

    flex.spacing = LoaderUtils::SafeStof(node.GetChildValue("spacing", "5.0"));
    flex.autoSize = node.GetChildValue("autoSize", "false") == "true" || node.GetChildValue("autoSize", "0") == "1";

    std::stringstream padSS(node.GetChildValue("padding", "0 0 0 0"));
    padSS >> flex.padding.x >> flex.padding.y >> flex.padding.z >> flex.padding.w;
}

void ComponentLoader::LoadUIInteractive(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Interactable"}, "UIInteractive");
    auto& interactive = scene.AddComponent<UIInteractiveComponent>(entity);
    const std::string value = node.GetChildValue("Interactable", "true");
    interactive.interactable = value == "true" || value == "1";
}

void ComponentLoader::LoadUIAnimation(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"Enabled", "AnimateColor", "AnimateScale", "NormalColor", "HoverColor", "PressedColor",
                               "NormalScale", "HoverScale", "PressedScale", "TransitionSpeed"},
                              "UIAnimation");
    auto& animation = scene.AddComponent<UIAnimationComponent>(entity);
    auto readBool = [&](const char* key, bool defaultValue) {
        const std::string value = node.GetChildValue(key, defaultValue ? "true" : "false");
        return value == "true" || value == "1";
    };
    animation.enabled = readBool("Enabled", true);
    animation.animateColor = readBool("AnimateColor", true);
    animation.animateScale = readBool("AnimateScale", false);
    std::stringstream normalColor(node.GetChildValue("NormalColor", "1 1 1 1"));
    normalColor >> animation.normalColor.r >> animation.normalColor.g >> animation.normalColor.b >>
        animation.normalColor.a;
    std::stringstream hoverColor(node.GetChildValue("HoverColor", "1 1 1 1"));
    hoverColor >> animation.hoverColor.r >> animation.hoverColor.g >> animation.hoverColor.b >> animation.hoverColor.a;
    std::stringstream pressedColor(node.GetChildValue("PressedColor", "0.85 0.85 0.85 1"));
    pressedColor >> animation.pressedColor.r >> animation.pressedColor.g >> animation.pressedColor.b >>
        animation.pressedColor.a;
    animation.normalScale = LoaderUtils::SafeStof(node.GetChildValue("NormalScale", "1"));
    animation.hoverScale = LoaderUtils::SafeStof(node.GetChildValue("HoverScale", "1"));
    animation.pressedScale = LoaderUtils::SafeStof(node.GetChildValue("PressedScale", "0.98"));
    animation.transitionSpeed = LoaderUtils::SafeStof(node.GetChildValue("TransitionSpeed", "12"));
    animation.currentScale = animation.normalScale;
    animation.visualScale = animation.normalScale;
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

    auto& lod = scene.AddComponent<LODComponent>(entity);

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
        lod.lodModelNames.push_back(modelName);
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
    LoaderUtils::ValidateKeys(node, {"Skybox", "Shader", "Primary"}, "SkyboxRenderer");

    auto& comp = scene.AddComponent<SkyboxRenderComponent>(entity);

    std::string skyboxName = node.GetChildValue("Skybox");
    std::string shaderName = node.GetChildValue("Shader");

    comp.skybox = res.GetSkybox(skyboxName);
    comp.skyboxName = skyboxName;

    comp.shader = res.GetShader(shaderName);
    comp.shaderName = shaderName;
    comp.isPrimary = node.GetChildValue("Primary", "true") == "true" || node.GetChildValue("Primary", "1") == "1";

    if (!comp.skybox)
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer skybox not found: " << skyboxName;
    if (comp.shader.expired())
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer shader not found: " << shaderName;
    if (comp.isPrimary)
        scene.SetActiveSkybox(entity);
}

void ComponentLoader::LoadReflectionProbe(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node, {"Type", "Resolution", "BoxProjection", "BoxMin", "BoxMax", "BlendDistance"},
                              "ReflectionProbe");

    auto& comp = scene.AddComponent<ReflectionProbeComponent>(entity);

    std::string typeStr = node.GetChildValue("Type", "Static");
    comp.type =
        (typeStr == "Dynamic" || typeStr == "DYNAMIC") ? ReflectionProbeType::Dynamic : ReflectionProbeType::Static;

    comp.resolution = LoaderUtils::SafeStoi(node.GetChildValue("Resolution", "512"));
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
    comp.blendDistance = LoaderUtils::SafeStof(node.GetChildValue("BlendDistance", "1.0"));
    comp.lastResolution = 0;  // Force cubemap allocation on first capture
}

void ComponentLoader::LoadAudioSource(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Audio", "Sound", "Path", "Volume", "Pitch", "Pan", "Speed", "Loop", "Is3d",
                               "MinDistance", "MaxDistance", "Velocity", "PlayOnAwake"},
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
        audio.source = res.GetSoundAuto(audioName);
        audio.resourceName = audioName;
        audio.filePath = node.GetChildValue("Path");
        std::string velStr = node.GetChildValue("Velocity", "0 0 0");
        std::stringstream ss(velStr);
        ss >> audio.velocity.x >> audio.velocity.y >> audio.velocity.z;
    }

    audio.playOnAwake =
        node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";
    audio.loop = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";
    audio.is3D = node.GetChildValue("Is3d", "0") == "1" || node.GetChildValue("Is3d", "false") == "true";
    audio.volume = LoaderUtils::SafeStof(node.GetChildValue("Volume", "100.0"));
    audio.pitch = LoaderUtils::SafeStof(node.GetChildValue("Pitch", "1.0"));
    audio.pan = LoaderUtils::SafeStof(node.GetChildValue("Pan", "0.0"));
    audio.speed = LoaderUtils::SafeStof(node.GetChildValue("Speed", "1.0"));
    audio.minDistance = LoaderUtils::SafeStof(node.GetChildValue("MinDistance", "1.0"));
    audio.maxDistance = LoaderUtils::SafeStof(node.GetChildValue("MaxDistance", "100.0"));

    scene.AddComponent<AudioSourceComponent>(entity, audio);
}

void ComponentLoader::LoadVideoPlayer(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Path", "Loop", "Speed", "PlayOnAwake", "Volume", "MaxDecodes"}, "VideoPlayer");

    VideoPlayerComponent video;
    video.filePath = node.GetChildValue("Path");
    video.isLooping = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";
    video.playOnAwake =
        node.GetChildValue("PlayOnAwake", "true") == "true" || node.GetChildValue("PlayOnAwake", "1") == "1";

    video.speed = LoaderUtils::SafeStof(node.GetChildValue("Speed", "1.0"));
    if (video.speed < 0.0f)
        LOGGER_WARN("ComponentLoader") << "VideoPlayer Speed must be positive: " << video.speed;

    video.volume = LoaderUtils::SafeStof(node.GetChildValue("Volume", "1.0"));
    video.maxDecodes = std::max(1, LoaderUtils::SafeStoi(node.GetChildValue("MaxDecodes", "1")));
    scene.AddComponent<VideoPlayerComponent>(entity, video);
}

void ComponentLoader::LoadMaterial(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(
        node, {"Opacity", "Roughness",   "Metallic",    "Albedo",   "Normal",      "MetallicMap", "RoughnessMap",
               "AO",      "EmissiveMap", "SpecularMap", "Emission", "AlphaCutoff", "UVScale",     "UVOffset",
               "AO_Map",  "AO_Path",     "BlendSrc",    "BlendDst", "Type",        "Ports"},
        "Material");
    auto& mat = scene.AddComponent<MaterialComponent>(entity);
    mat.desc.opacity = LoaderUtils::SafeStof(node.GetChildValue("Opacity", "1.0"));
    mat.desc.pbr.roughness = LoaderUtils::SafeStof(node.GetChildValue("Roughness", "0.5"));
    mat.desc.pbr.metallic = LoaderUtils::SafeStof(node.GetChildValue("Metallic", "0.0"));
    mat.desc.pbr.ao = LoaderUtils::SafeStof(node.GetChildValue("AO", "1.0"));
    mat.desc.alphaCutoff = LoaderUtils::SafeStof(node.GetChildValue("AlphaCutoff", "0.5"));

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

    constexpr int maxBlendFactor = static_cast<int>(BlendFactor::OneMinusConstantAlpha);
    mat.desc.blendSrc = static_cast<BlendFactor>(std::clamp(
        LoaderUtils::SafeStoi(node.GetChildValue("BlendSrc", std::to_string(static_cast<int>(BlendFactor::SrcAlpha)))),
        0, maxBlendFactor));
    mat.desc.blendDst = static_cast<BlendFactor>(
        std::clamp(LoaderUtils::SafeStoi(
                       node.GetChildValue("BlendDst", std::to_string(static_cast<int>(BlendFactor::OneMinusSrcAlpha)))),
                   0, maxBlendFactor));
    mat.desc.type = node.GetChildValue("Type", "PBR");
    std::stringstream ports(node.GetChildValue("Ports"));
    for (float& port : mat.desc.ports.data)
    {
        if (!(ports >> port))
            break;
    }

    auto resolveTexture = [&](const std::string& nameOrPath) {
        auto texture = res.GetTextureAuto(nameOrPath);
        return texture ? texture->id : 0u;
    };
    mat.gpu.albedoMap = resolveTexture(mat.desc.albedoPath);
    mat.gpu.normalMap = resolveTexture(mat.desc.normalPath);
    mat.gpu.metallicMap = resolveTexture(mat.desc.metallicPath);
    mat.gpu.roughnessMap = resolveTexture(mat.desc.roughnessPath);
    mat.gpu.emissiveMap = resolveTexture(mat.desc.emissivePath);
    mat.gpu.aoMap = resolveTexture(mat.desc.aoPath);
    mat.gpu.specularMap = resolveTexture(mat.desc.specularPath);
    mat.gpu.dirty = false;
}

void ComponentLoader::LoadFragment(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    auto& frag = scene.AddComponent<FragmentComponent>(entity);
    frag.path = node.GetChildValue("Path");

    // Check for both "Override" (singular) and "Overrides" (plural)
    const YAMLNode* overrideNode = node.GetChild("Override");
    if (!overrideNode)
        overrideNode = node.GetChild("Overrides");

    if (overrideNode)
        frag.overrides = SerializeYAML(*overrideNode);
}

void ComponentLoader::LoadPathFollower(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"MoveSpeed",
                               "RotationSpeed",
                               "MaxRotationSpeed",
                               "RotationAcceleration",
                               "RotationOffset",
                               "ArrivalDistance",
                               "RecordDebugPath",
                               "Provider",
                               "Criteria",
                               "PreferredTags",
                               "TagWeightBonus",
                               "AltitudePenaltyWeight",
                               "ProviderEntity",
                               "LockXPitch",
                               "LockYYaw",
                               "LockZRoll",
                               "LockMoveX",
                               "LockMoveY",
                               "LockMoveZ",
                               "LocalAvoidance",
                               "SeparationRadius",
                               "SeparationWeight",
                               "ObstacleAvoidanceDistance",
                               "ObstacleAvoidanceWeight"},
                              "PathFollower");
    auto& pf = scene.AddComponent<PathFollowerComponent>(entity);
    pf.moveSpeed = LoaderUtils::SafeStof(node.GetChildValue("MoveSpeed", "5.0"));
    pf.rotationSpeed = LoaderUtils::SafeStof(node.GetChildValue("RotationSpeed", "10.0"));
    pf.maxRotationSpeed = LoaderUtils::SafeStof(node.GetChildValue("MaxRotationSpeed", "20.0"));
    pf.rotationAcceleration = LoaderUtils::SafeStof(node.GetChildValue("RotationAcceleration", "40.0"));
    std::stringstream offset(node.GetChildValue("RotationOffset", "0 0 0"));
    offset >> pf.rotationOffset.x >> pf.rotationOffset.y >> pf.rotationOffset.z;
    pf.arrivalDistance = LoaderUtils::SafeStof(node.GetChildValue("ArrivalDistance", "0.5"));
    const auto readBool = [&](const char* key, bool defaultValue) {
        const std::string value = node.GetChildValue(key, defaultValue ? "true" : "false");
        return value == "true" || value == "1";
    };
    pf.recordDebugPath = readBool("RecordDebugPath", true);
    pf.pathfindingOptions.provider =
        static_cast<NavigationProvider>(std::clamp(LoaderUtils::SafeStoi(node.GetChildValue("Provider", "0")), 0, 2));
    pf.pathfindingOptions.criteria =
        static_cast<PathfindingCriteria>(std::clamp(LoaderUtils::SafeStoi(node.GetChildValue("Criteria", "0")), 0, 5));
    pf.pathfindingOptions.preferredTags.clear();
    std::string preferredTags = node.GetChildValue("PreferredTags", "walkable");
    std::replace(preferredTags.begin(), preferredTags.end(), ',', ' ');
    std::stringstream tags(preferredTags);
    for (std::string tag; tags >> tag;) pf.pathfindingOptions.preferredTags.push_back(std::move(tag));
    pf.pathfindingOptions.tagWeightBonus = LoaderUtils::SafeStof(node.GetChildValue("TagWeightBonus", "5.0"));
    pf.pathfindingOptions.altitudePenaltyWeight =
        LoaderUtils::SafeStof(node.GetChildValue("AltitudePenaltyWeight", "10.0"));
    pf.navigationProviderName = node.GetChildValue("ProviderEntity");
    pf.lockXPitch = readBool("LockXPitch", false);
    pf.lockYYaw = readBool("LockYYaw", false);
    pf.lockZRoll = readBool("LockZRoll", false);
    pf.lockMoveX = readBool("LockMoveX", false);
    pf.lockMoveY = readBool("LockMoveY", false);
    pf.lockMoveZ = readBool("LockMoveZ", false);
    pf.localAvoidanceEnabled = readBool("LocalAvoidance", true);
    pf.separationRadius = LoaderUtils::SafeStof(node.GetChildValue("SeparationRadius", "1.25"));
    pf.separationWeight = LoaderUtils::SafeStof(node.GetChildValue("SeparationWeight", "0.85"));
    pf.obstacleAvoidanceDistance = LoaderUtils::SafeStof(node.GetChildValue("ObstacleAvoidanceDistance", "1.8"));
    pf.obstacleAvoidanceWeight = LoaderUtils::SafeStof(node.GetChildValue("ObstacleAvoidanceWeight", "1.0"));
}

void ComponentLoader::LoadNavMesh(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node,
                              {"IsDynamic", "NeedsRebuild", "TerrainGridResolution", "WalkableNormalY",
                               "CarveHeightPadding", "CarveAgentRadius", "Vertices", "Triangles", "Nodes"},
                              "NavMesh");
    auto& nav = scene.AddOrReplaceComponent<NavMeshComponent>(entity);
    nav.isDynamic = ParseBoolValue(node.GetChildValue("IsDynamic", "false"));
    nav.needsRebuild = ParseBoolValue(node.GetChildValue("NeedsRebuild", "true"), true);
    nav.terrainGridResolution = std::max(1, LoaderUtils::SafeStoi(node.GetChildValue("TerrainGridResolution", "64")));
    nav.walkableNormalY = LoaderUtils::SafeStof(node.GetChildValue("WalkableNormalY", "0.3"));
    nav.carveHeightPadding = std::max(0.0f, LoaderUtils::SafeStof(node.GetChildValue("CarveHeightPadding", "0.5")));
    nav.carveAgentRadius = std::max(0.0f, LoaderUtils::SafeStof(node.GetChildValue("CarveAgentRadius", "0.0")));

    nav.vertices.clear();
    if (const auto* vertices = node.GetChild("Vertices"))
    {
        nav.vertices.reserve(vertices->children.size());
        for (const auto& vertex : vertices->children)
        {
            if (vertex.key == "Vertex")
                nav.vertices.push_back(ParseVec3Value(vertex.value));
        }
    }

    nav.triangles.clear();
    if (const auto* triangles = node.GetChild("Triangles"))
    {
        nav.triangles.reserve(triangles->children.size());
        for (const auto& triangleNode : triangles->children)
        {
            if (triangleNode.key != "Triangle")
                continue;
            NavMeshTriangle triangle{};
            std::stringstream indices(triangleNode.GetChildValue("Indices"));
            indices >> triangle.indices[0] >> triangle.indices[1] >> triangle.indices[2];
            triangle.center = ParseVec3Value(triangleNode.GetChildValue("Center"));
            triangle.normal = ParseVec3Value(triangleNode.GetChildValue("Normal"), glm::vec3(0.0f, 1.0f, 0.0f));
            triangle.tag = triangleNode.GetChildValue("Tag", "walkable");
            nav.triangles.push_back(std::move(triangle));
        }
    }

    nav.nodes.clear();
    if (const auto* nodes = node.GetChild("Nodes"))
    {
        nav.nodes.reserve(nodes->children.size());
        for (const auto& nodeData : nodes->children)
        {
            if (nodeData.key != "Node")
                continue;
            NavMeshNode navNode{};
            navNode.position = ParseVec3Value(nodeData.GetChildValue("Position"));
            navNode.triangleIndex = LoaderUtils::SafeStoul(nodeData.GetChildValue("TriangleIndex", "0"));
            navNode.tag = nodeData.GetChildValue("Tag", "walkable");
            std::stringstream neighbors(nodeData.GetChildValue("Neighbors"));
            uint32_t neighbor = 0;
            while (neighbors >> neighbor) navNode.neighbors.push_back(neighbor);
            nav.nodes.push_back(std::move(navNode));
        }
    }
}

void ComponentLoader::LoadNavigationGrid(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Origin", "Width", "Height", "CellSize", "AllowDiagonal", "Cells"},
                              "NavigationGrid");
    auto& grid = scene.AddOrReplaceComponent<NavigationGridComponent>(entity);
    grid.origin = ParseVec3Value(node.GetChildValue("Origin"));
    grid.width = std::max(0, LoaderUtils::SafeStoi(node.GetChildValue("Width", "0")));
    grid.height = std::max(0, LoaderUtils::SafeStoi(node.GetChildValue("Height", "0")));
    grid.cellSize = std::max(0.0001f, LoaderUtils::SafeStof(node.GetChildValue("CellSize", "1.0")));
    grid.allowDiagonal = ParseBoolValue(node.GetChildValue("AllowDiagonal", "false"));
    grid.cells.clear();
    if (const auto* cells = node.GetChild("Cells"))
    {
        grid.cells.reserve(cells->children.size());
        for (const auto& cellNode : cells->children)
        {
            if (cellNode.key != "Cell")
                continue;
            NavigationGridCell cell;
            cell.walkable = ParseBoolValue(cellNode.GetChildValue("Walkable", "true"), true);
            cell.cost = std::max(0.0f, LoaderUtils::SafeStof(cellNode.GetChildValue("Cost", "1.0")));
            cell.tag = cellNode.GetChildValue("Tag", "walkable");
            grid.cells.push_back(std::move(cell));
        }
    }
}

void ComponentLoader::LoadDecal(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(node,
                              {"Albedo", "Opacity", "Roughness", "Metallic", "Reflectivity", "TintColor", "Lifetime",
                               "RenderOrder", "LightingMode", "TargetTags", "Shader"},
                              "Decal");
    auto& d = scene.AddComponent<DecalComponent>(entity);
    d.albedoTexture = node.GetChildValue("Albedo");
    if (auto texture = res.GetTextureAuto(d.albedoTexture))
        d.albedoMap = texture->id;
    d.opacity = LoaderUtils::SafeStof(node.GetChildValue("Opacity", "1.0"));
    d.roughness = LoaderUtils::SafeStof(node.GetChildValue("Roughness", "1.0"));
    d.metallic = LoaderUtils::SafeStof(node.GetChildValue("Metallic", "0.0"));
    d.reflectivity = LoaderUtils::SafeStof(node.GetChildValue("Reflectivity", "0.0"));
    d.lifetime = LoaderUtils::SafeStof(node.GetChildValue("Lifetime", "-1.0"));
    d.renderOrder = static_cast<uint32_t>(std::max(0, LoaderUtils::SafeStoi(node.GetChildValue("RenderOrder", "0"))));
    d.lightingMode = std::clamp(LoaderUtils::SafeStoi(node.GetChildValue("LightingMode", "0")), 0, 2);

    std::stringstream ss(node.GetChildValue("TintColor", "1 1 1 1"));
    ss >> d.tintColor.r >> d.tintColor.g >> d.tintColor.b >> d.tintColor.a;
    std::string targetTags = node.GetChildValue("TargetTags");
    std::replace(targetTags.begin(), targetTags.end(), ',', ' ');
    std::istringstream tagStream(targetTags);
    for (std::string tag; tagStream >> tag;) d.targetTags.push_back(std::move(tag));
    d.customShader = node.GetChildValue("Shader");
}

void ComponentLoader::LoadPlanarReflection(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Resolution", "ResolutionY", "ResolutionScale", "UpdateIntervalFrames", "Normal"},
                              "PlanarReflection");
    auto& pr = scene.AddComponent<PlanarReflectionComponent>(entity);
    pr.resolution = LoaderUtils::SafeStoi(node.GetChildValue("Resolution", "1024"));
    pr.resolution_y = LoaderUtils::SafeStoi(node.GetChildValue("ResolutionY", std::to_string(pr.resolution)));
    pr.resolutionScale = std::clamp(LoaderUtils::SafeStof(node.GetChildValue("ResolutionScale", "0.5")), 0.1f, 1.0f);
    pr.updateIntervalFrames = static_cast<uint32_t>((std::max)(
        1, LoaderUtils::SafeStoi(node.GetChildValue("UpdateIntervalFrames", "1"))));
    std::stringstream ss(node.GetChildValue("Normal", "0 1 0"));
    ss >> pr.normal.x >> pr.normal.y >> pr.normal.z;
}

void ComponentLoader::LoadLightProbe(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Intensity", "Radius", "Tint", "SH"}, "LightProbe");
    auto& lp = scene.AddComponent<LightProbeComponent>(entity);
    lp.intensity = LoaderUtils::SafeStof(node.GetChildValue("Intensity", "1.0"));
    lp.radius = LoaderUtils::SafeStof(node.GetChildValue("Radius", "5.0"));
    std::stringstream tintStream(node.GetChildValue("Tint", "1 1 1"));
    tintStream >> lp.tint.x >> lp.tint.y >> lp.tint.z;
    std::stringstream shStream(node.GetChildValue("SH"));
    for (glm::vec3& coefficient : lp.sh)
    {
        if (!(shStream >> coefficient.x >> coefficient.y >> coefficient.z))
            break;
    }
}

void ComponentLoader::LoadTerrain(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res)
{
    LoaderUtils::ValidateKeys(
        node,
        {"HeightMap", "SplatMap", "DiffuseLayers", "NormalLayers", "Size", "MaxHeight", "Resolution", "ChunkSize",
         "LODDistances", "TextureScale", "CastShadows", "GeneratePhysics", "Walkable", "Shader"},
        "Terrain");
    auto& t = scene.AddComponent<TerrainComponent>(entity);
    t.heightMapName = node.GetChildValue("HeightMap");
    t.splatMapName = node.GetChildValue("SplatMap");
    auto parseNames = [](std::string value, std::vector<std::string>& names) {
        std::replace(value.begin(), value.end(), ',', ' ');
        std::stringstream stream(value);
        names.clear();
        for (std::string name; stream >> name;) names.push_back(std::move(name));
    };
    parseNames(node.GetChildValue("DiffuseLayers"), t.diffuseLayerNames);
    parseNames(node.GetChildValue("NormalLayers"), t.normalLayerNames);
    std::stringstream ss(node.GetChildValue("Size", "512 50 512"));
    ss >> t.terrainSize.x >> t.terrainSize.y >> t.terrainSize.z;
    t.maxHeight = LoaderUtils::SafeStof(node.GetChildValue("MaxHeight", std::to_string(t.terrainSize.y)));
    t.resolution = LoaderUtils::SafeStoi(node.GetChildValue("Resolution", "1024"));
    t.chunkSize = LoaderUtils::SafeStoi(node.GetChildValue("ChunkSize", "64"));
    std::stringstream lodDistances(node.GetChildValue("LODDistances", "75 150 300"));
    lodDistances >> t.lodDistances.x >> t.lodDistances.y >> t.lodDistances.z;
    t.lodDistances.x = std::max(0.0f, t.lodDistances.x);
    t.lodDistances.y = std::max(t.lodDistances.x, t.lodDistances.y);
    t.lodDistances.z = std::max(t.lodDistances.y, t.lodDistances.z);
    t.textureScale = LoaderUtils::SafeStof(node.GetChildValue("TextureScale", "1.0"));
    t.castShadows = node.GetChildValue("CastShadows", "true") == "true";
    t.generatePhysics = node.GetChildValue("GeneratePhysics", "false") == "true";
    t.isWalkable = node.GetChildValue("Walkable", "false") == "true";
    t.customShader = node.GetChildValue("Shader");

    if (auto texture = res.GetTexture(t.heightMapName))
        t.heightMap = texture->id;
    if (auto texture = res.GetTexture(t.splatMapName))
        t.splatMap = texture->id;
    for (const std::string& name : t.diffuseLayerNames)
    {
        if (auto texture = res.GetTexture(name))
            t.diffuseLayers.push_back(texture->id);
    }
    for (const std::string& name : t.normalLayerNames)
    {
        if (auto texture = res.GetTexture(name))
            t.normalLayers.push_back(texture->id);
    }
}

void ComponentLoader::LoadNetwork(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"NetworkId", "OwnerId", "IsLocal", "ReplicateTransform", "InterestRadius"},
                              "Network");
    auto& net = scene.AddComponent<NetworkComponent>(entity);
    net.networkId = LoaderUtils::SafeStoul(node.GetChildValue("NetworkId", "0"));
    net.ownerId = LoaderUtils::SafeStoul(node.GetChildValue("OwnerId", "0"));
    net.isLocal = node.GetChildValue("IsLocal", "false") == "true";
    net.replicateTransform = node.GetChildValue("ReplicateTransform", "true") != "false";
    net.interestRadius = std::max(0.0f, LoaderUtils::SafeStof(node.GetChildValue("InterestRadius", "0")));
}

void ComponentLoader::LoadOcclusion(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Visible"}, "Occlusion");
    auto& occlusion = scene.AddComponent<OcclusionComponent>(entity);
    const std::string value = node.GetChildValue("Visible", "true");
    occlusion.isVisible = value == "true" || value == "1";
}

void ComponentLoader::LoadStreaming(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Model", "Static", "LoadDistance", "UnloadDistance"}, "Streaming");
    auto& streaming = scene.AddComponent<StreamingComponent>(entity);
    streaming.modelPath = node.GetChildValue("Model");
    const std::string staticValue = node.GetChildValue("Static", "false");
    streaming.isStatic = staticValue == "true" || staticValue == "1";
    streaming.loadDistance = std::max(0.0f, LoaderUtils::SafeStof(node.GetChildValue("LoadDistance", "100")));
    streaming.unloadDistance =
        std::max(streaming.loadDistance, LoaderUtils::SafeStof(node.GetChildValue("UnloadDistance", "150")));
}
