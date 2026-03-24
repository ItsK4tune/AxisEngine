#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/decal_component.h>
#include <resource/unit/animator.h>
#include <audio/logic/audio_service.h>
#include <algorithm>
#include <core/app/application.h>
#include <ecs/logic/entity_manager.h>
#include <iostream>
#include <scene/logic/component_loader.h>
#include <core/logic/loader_utils.h>
#include <core/logic/service_locator.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <physics/logic/physics_loader.h>
#include <navigation/unit/pathfollower_component.h>

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

bool ComponentLoader::Load(const std::string& type, Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld *phys)
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
    if (s_Initialized) return;
    s_Initialized = true;

    RegisterLoader("Renderer", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadRenderer(s, e, n, r); });
    RegisterLoader("Animator", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadAnimator(s, e, n, r); });
    RegisterLoader("Camera", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadCamera(s, e, n); });
    RegisterLoader("LightDir", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadLightDir(s, e, n); });
    RegisterLoader("LightPoint", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadLightPoint(s, e, n); });
    RegisterLoader("LightSpot", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadLightSpot(s, e, n); });
    RegisterLoader("UITransform", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadUITransform(s, e, n); });
    RegisterLoader("UIRenderer", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadUIRenderer(s, e, n, r); });
    RegisterLoader("UIText", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadUIText(s, e, n, r); });
    RegisterLoader("SkyboxRenderer", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadSkyboxRenderer(s, e, n, r); });

    RegisterLoader("AudioSource", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadAudioSource(s, e, n, r); });
    RegisterLoader("VideoPlayer", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadVideoPlayer(s, e, n); });
    RegisterLoader("ParticleEmitter", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadParticleEmitter(s, e, n, r); });
    RegisterLoader("Material", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadMaterial(s, e, n, r); });
    RegisterLoader("LOD", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadLOD(s, e, n, r); });
    RegisterLoader("RigidBody", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { if (p) PhysicsLoader::LoadRigidBody(s, e, n, *p); });
    RegisterLoader("CharacterController", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { if (p) PhysicsLoader::LoadCharacterController(s, e, n, *p); });
    RegisterLoader("Transform", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) {  });
    RegisterLoader("PathFollower", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { ComponentLoader::LoadPathFollower(s, e, n); });
    RegisterLoader("Decal", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadDecal(s, e, n, r); });
    RegisterLoader("UIFlex", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) { LoadUIFlex(s, e, n); });
}

void ComponentLoader::LoadRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Model", "Shader", "Order", "Color", "CastShadow", "ReceiveShadow"}, "Renderer");

    std::string modelName = node.GetChildValue("Model");
    std::string shaderName = node.GetChildValue("Shader");
    int order = std::stoi(node.GetChildValue("Order", "0"));
    bool castShadow = node.GetChildValue("CastShadow", "1") == "1" || node.GetChildValue("CastShadow", "true") == "true";
    bool receiveShadow = node.GetChildValue("ReceiveShadow", "1") == "1" || node.GetChildValue("ReceiveShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1 1"));
    float cr = 1, cg = 1, cb = 1, ca = 1;
    colorSS >> cr >> cg >> cb >> ca;
    glm::vec4 color(cr, cg, cb, ca);

    if (modelName.empty() || shaderName.empty())
    {
        LOGGER_WARN("ComponentLoader") << "Renderer component missing 'Model' or 'Shader' field";
    }

    auto &r = scene.registry.emplace<MeshRendererComponent>(entity);
    

    r.model = res.GetModelAuto(modelName, false);


    r.shader = res.GetShader(shaderName);
    if (r.shader.expired()) {


    }

    r.order = order;
    r.castShadow = castShadow;
    r.receiveShadow = receiveShadow;
    r.color = color;

    if (!r.model)
        LOGGER_WARN("ComponentLoader") << "Renderer model not found: " << modelName;
    if (r.shader.expired())
        LOGGER_WARN("ComponentLoader") << "Renderer shader not found: " << shaderName;
}

void ComponentLoader::LoadAnimator(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Animation", "Speed", "StartTime", "Rate"}, "Animator");

    auto &a = scene.registry.emplace<AnimationComponent>(entity);

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
                auto &mrc = scene.registry.get<MeshRendererComponent>(entity);
                if (mrc.model && mrc.model->IsStatic())
                {
                    LOGGER_WARN("ComponentLoader") << "Entity has Animator but its Model is STATIC! Animations will not play correctly. Entity: "
                                                   << (scene.registry.all_of<InfoComponent>(entity) ? scene.registry.get<InfoComponent>(entity).name : "Unknown");
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

void ComponentLoader::LoadCamera(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"Primary", "FOV", "Yaw", "Pitch", "Near", "Far", "AspectRatio"}, "Camera");

    auto &c = scene.registry.emplace<CameraComponent>(entity);
    c.isPrimary = node.GetChildValue("Primary", "1") == "1" || node.GetChildValue("Primary", "true") == "true";

    c.aspectRatio = std::stof(node.GetChildValue("AspectRatio", "0.0"));

    c.fov = std::stof(node.GetChildValue("FOV", "45.0"));
    if (c.fov <= 0.0f || c.fov >= 180.0f)
        LOGGER_WARN("ComponentLoader") << "Camera FOV out of bounds (0-180): " << c.fov;

    c.yaw = std::stof(node.GetChildValue("Yaw", "-90.0"));

    c.pitch = std::stof(node.GetChildValue("Pitch", "0.0"));
    if (c.pitch < -89.0f || c.pitch > 89.0f)
        LOGGER_WARN("ComponentLoader") << "Camera Pitch out of bounds (-89 to 89): " << c.pitch;

    c.nearPlane = std::stof(node.GetChildValue("Near", "0.1"));
    c.farPlane = std::stof(node.GetChildValue("Far", "1000.0"));

    if (c.nearPlane <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane should be > 0";
    if (c.nearPlane >= c.farPlane)
        LOGGER_WARN("ComponentLoader") << "Camera Near plane must be less than Far plane";

    if (c.isPrimary)
        EntityManager::SetActiveCamera(scene, entity);
}

void ComponentLoader::LoadLightDir(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "Ambient", "Diffuse", "Specular"}, "LightDir");

    auto &l = scene.registry.emplace<DirectionalLightComponent>(entity);

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

void ComponentLoader::LoadLightPoint(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "Radius", "Constant", "Linear", "Quadratic", "Ambient", "Diffuse", "Specular"}, "LightPoint");

    auto &l = scene.registry.emplace<PointLightComponent>(entity);

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

void ComponentLoader::LoadLightSpot(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "CutOff", "OuterCutOff", "Constant", "Linear", "Quadratic", "Ambient", "Diffuse", "Specular"}, "LightSpot");

    auto &l = scene.registry.emplace<SpotLightComponent>(entity);

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

void ComponentLoader::LoadUITransform(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    auto &ui = scene.registry.emplace<UITransformComponent>(entity);

    std::stringstream posSS(node.GetChildValue("Position", "0 0"));
    posSS >> ui.position.x >> ui.position.y;

    std::stringstream sizeSS(node.GetChildValue("Size", "100 100"));
    sizeSS >> ui.size.x >> ui.size.y;

    ui.zIndex = std::stoi(node.GetChildValue("ZOrder", "0"));
    std::string zLabel = node.GetChildValue("zIndex");
    if (!zLabel.empty()) ui.zIndex = std::stoi(zLabel);
    
    ui.usePercentage = node.GetChildValue("UsePercentage", "0") == "1" || node.GetChildValue("UsePercentage", "true") == "true";

    std::stringstream aminSS(node.GetChildValue("anchorMin", "0.5 0.5"));
    aminSS >> ui.anchorMin.x >> ui.anchorMin.y;
    
    std::stringstream amaxSS(node.GetChildValue("anchorMax", "0.5 0.5"));
    amaxSS >> ui.anchorMax.x >> ui.anchorMax.y;

    std::stringstream ominSS(node.GetChildValue("offsetMin", "-50 -50"));
    ominSS >> ui.offsetMin.x >> ui.offsetMin.y;

    std::stringstream omaxSS(node.GetChildValue("offsetMax", "50 50"));
    omaxSS >> ui.offsetMax.x >> ui.offsetMax.y;

    std::stringstream pivotSS(node.GetChildValue("pivot", "0.5 0.5"));
    pivotSS >> ui.pivot.x >> ui.pivot.y;
}

void ComponentLoader::LoadUIRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    auto &ui = scene.registry.emplace<UIRendererComponent>(entity);

    std::stringstream colorSS(node.GetChildValue("color", "1 1 1 1"));
    if (node.GetChildValue("color").empty()) colorSS.str(node.GetChildValue("Color", "1 1 1 1"));
    colorSS >> ui.color.r >> ui.color.g >> ui.color.b >> ui.color.a;

    auto StripQuotes = [](std::string s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.length() - 2);
        return s;
    };

    std::string textureName = StripQuotes(node.GetChildValue("texture"));
    if (textureName.empty()) textureName = StripQuotes(node.GetChildValue("Texture"));
    
    if (!textureName.empty()) {
        std::shared_ptr<Texture> tex = res.GetTextureAuto(textureName);
        std::string finalName = textureName;

        if (!res.GetUIModel(finalName)) res.CreateUIModel(finalName, ::UIType::Texture);
        auto model = res.GetUIModel(finalName);
        if (tex && model) {
            model->SetTexture(tex->id);
            ui.texture = tex;
        }
        ui.model = model;
    } else {
        if (!res.GetUIModel("default_rect")) res.CreateUIModel("default_rect", ::UIType::Color);
        ui.model = res.GetUIModel("default_rect");
    }

    std::string shaderName = StripQuotes(node.GetChildValue("shader"));
    if (shaderName.empty()) shaderName = StripQuotes(node.GetChildValue("Shader", "uiShader"));
    ui.shader = res.GetShader(shaderName);
}

void ComponentLoader::LoadUIText(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    auto &txt = scene.registry.emplace<UITextComponent>(entity);

    auto StripQuotes = [](std::string s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.length() - 2);
        return s;
    };

    std::string textContent = node.GetChildValue("text");
    if (textContent.empty()) textContent = node.GetChildValue("Text");
    txt.text = StripQuotes(textContent);

    std::string fontName = StripQuotes(node.GetChildValue("font"));
    if (fontName.empty()) fontName = StripQuotes(node.GetChildValue("Font"));
    
    if (!fontName.empty()) {
        txt.font = res.GetFontAuto(fontName, 60);
    }
    
    std::stringstream colorSS(node.GetChildValue("color", "1 1 1 1"));
    colorSS >> txt.color.r >> txt.color.g >> txt.color.b >> txt.color.a;

    txt.scale = std::stof(node.GetChildValue("scale", "1.0"));

    std::string alignStr = node.GetChildValue("alignment", "Left");
    if (alignStr == "Center") txt.alignment = TextAlignment::Center;
    else if (alignStr == "Right") txt.alignment = TextAlignment::Right;
    else txt.alignment = TextAlignment::Left;

    txt.wordWrap = node.GetChildValue("wordWrap", "0") == "1" || node.GetChildValue("wordWrap", "true") == "true";
    txt.maxWidth = std::stof(node.GetChildValue("maxWidth", "0.0"));

    if (!res.GetUIModel("default_text_rect"))
        res.CreateUIModel("default_text_rect", ::UIType::Text);
    txt.model = res.GetUIModel("default_text_rect");
    txt.shader = res.GetShader("textShader");
}

void ComponentLoader::LoadUIFlex(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    auto &flex = scene.registry.emplace<UIFlexLayoutComponent>(entity);
    
    std::string dirStr = node.GetChildValue("direction", "Column");
    flex.direction = (dirStr == "Row") ? FlexDirection::Row : FlexDirection::Column;
    
    flex.spacing = std::stof(node.GetChildValue("spacing", "5.0"));
    
    std::stringstream padSS(node.GetChildValue("padding", "0 0 0 0"));
    padSS >> flex.padding.x >> flex.padding.y >> flex.padding.z >> flex.padding.w;
}

void ComponentLoader::LoadLOD(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Models", "Distances"}, "LOD");

    std::string modelsStr = node.GetChildValue("Models");
    std::string distancesStr = node.GetChildValue("Distances");

    if (modelsStr.empty() || distancesStr.empty())
    {
        LOGGER_WARN("ComponentLoader") << "LOD component missing 'Models' or 'Distances' field";
        return;
    }

    auto &lod = scene.registry.emplace<LODComponent>(entity);

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

void ComponentLoader::LoadSkyboxRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Skybox", "Shader"}, "SkyboxRenderer");

    auto &comp = scene.registry.emplace<SkyboxRenderComponent>(entity);

    std::string skyboxName = node.GetChildValue("Skybox");
    std::string shaderName = node.GetChildValue("Shader");

    comp.skybox = res.GetSkybox(skyboxName);
    if (!comp.skybox && skyboxName.find('|') != std::string::npos) {


    }

    comp.shader = res.GetShader(shaderName);

    if (!comp.skybox)
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer skybox not found: " << skyboxName;
    if (comp.shader.expired())
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer shader not found: " << shaderName;
    EntityManager::SetActiveSkybox(scene, entity);
}


void ComponentLoader::LoadAudioSource(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Audio", "Path", "Volume", "Pitch", "Pan", "Speed", "Loop", "Is3d", "MinDistance", "MaxDistance", "Velocity", "PlayOnAwake"}, "AudioSource");

    AudioSourceComponent audio;
    
    std::string audioName = node.GetChildValue("Audio");
    if (audioName.empty()) audioName = node.GetChildValue("Path");

    if (audioName.empty()) {
        LOGGER_WARN("ComponentLoader") << "AudioSource missing 'Audio' property";
    } else {
        auto audioSvc = ServiceLocator::Instance().Resolve<AudioService>();
        IAudioEngine* engine = audioSvc ? audioSvc->GetEngine() : nullptr;
        audio.source = res.GetSoundAuto(audioName, engine);
        audio.resourceName = audioName;
    }

    audio.volume = std::stof(node.GetChildValue("Volume", "1.0"));
    audio.pitch = std::stof(node.GetChildValue("Pitch", "1.0"));
    audio.pan = std::stof(node.GetChildValue("Pan", "0.0"));
    audio.speed = std::stof(node.GetChildValue("Speed", "1.0"));
    
    audio.loop = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";
    audio.is3D = node.GetChildValue("Is3d", "0") == "1" || node.GetChildValue("Is3d", "true") == "true";

    if (audio.is3D) {
        audio.minDistance = std::stof(node.GetChildValue("MinDistance", "1.0"));
        audio.maxDistance = std::stof(node.GetChildValue("MaxDistance", "100.0"));
        
        std::string velStr = node.GetChildValue("Velocity", "0 0 0");
        std::stringstream ss(velStr);
        ss >> audio.velocity.x >> audio.velocity.y >> audio.velocity.z;
    }
    
    audio.playOnAwake = node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";

    scene.registry.emplace<AudioSourceComponent>(entity, audio);
}

void ComponentLoader::LoadVideoPlayer(Scene &scene, entt::entity entity, const YAMLNode &node)
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
    
    video.playOnAwake = node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";

    scene.registry.emplace<VideoPlayerComponent>(entity, video);
}

void ComponentLoader::LoadParticleEmitter(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Texture", "MaxParticles", "Life", "SpawnRate", "StartColor", "EndColor", "StartSize", "EndSize", "MinVelocity", "MaxVelocity", "Shape"}, "ParticleEmitter");

    std::string texName = node.GetChildValue("Texture");

    int maxParticles = std::stoi(node.GetChildValue("MaxParticles", "100"));
    if (maxParticles <= 0)
        LOGGER_WARN("ComponentLoader") << "ParticleEmitter MaxParticles must be > 0: " << maxParticles;

    float life = std::stof(node.GetChildValue("Life", "1.0"));
    if (life <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "ParticleEmitter Life must be > 0: " << life;

    auto &emitterComp = scene.registry.emplace<ParticleEmitterComponent>(entity);
    emitterComp.emitter.Initialize(maxParticles);
    emitterComp.emitter.LifeTime = life;
    emitterComp.emitter.StartLife = life;

    emitterComp.emitter.SpawnRate = std::stof(node.GetChildValue("SpawnRate", "10.0"));
    emitterComp.emitter.StartSize = std::stof(node.GetChildValue("StartSize", "1.0"));
    emitterComp.emitter.EndSize = std::stof(node.GetChildValue("EndSize", "0.0"));

    auto parseVec3 = [](const std::string &val, const glm::vec3 &def) {
        std::stringstream ss(val);
        glm::vec3 r = def;
        ss >> r.x >> r.y >> r.z;
        return r;
    };

    auto parseVec4 = [](const std::string &val, const glm::vec4 &def) {
        std::stringstream ss(val);
        glm::vec4 r = def;
        ss >> r.x >> r.y >> r.z >> r.w;
        return r;
    };

    emitterComp.emitter.StartColor = parseVec4(node.GetChildValue("StartColor", "1 1 1 1"), glm::vec4(1.0f));
    emitterComp.emitter.EndColor = parseVec4(node.GetChildValue("EndColor", "1 1 1 0"), glm::vec4(1, 1, 1, 0));
    emitterComp.emitter.MinVelocity = parseVec3(node.GetChildValue("MinVelocity", "-0.1 1 -0.1"), glm::vec3(-0.1f, 1.0f, -0.1f));
    emitterComp.emitter.MaxVelocity = parseVec3(node.GetChildValue("MaxVelocity", "0.1 4 0.1"), glm::vec3(0.1f, 4.0f, 0.1f));

    std::string shapeStr = node.GetChildValue("Shape", "DIRECTIONAL");
    if (shapeStr == "CONE") emitterComp.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
    else if (shapeStr == "FIGURE_EIGHT") emitterComp.emitter.Shape = ParticleEmitter::EmissionShape::FIGURE_EIGHT;
    else emitterComp.emitter.Shape = ParticleEmitter::EmissionShape::DIRECTIONAL;

    auto tex = res.GetTextureAuto(texName);
    emitterComp.emitter.Texture = tex;

    if (!emitterComp.emitter.Texture)
    {
        LOGGER_ERROR("ComponentLoader") << "Particle Texture not found: " << texName;
    }
}

void ComponentLoader::LoadMaterial(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Type", "Roughness", "Metallic", "AO", "Shininess", "Specular", "Emission", "Ambient", "Opacity", "AlphaCutoff", "BlendSrc", "BlendDst", "Albedo", "Diffuse", "Normal", "MetallicMap", "RoughnessMap", "AOMap", "EmissiveMap", "UVScale", "UVOffset"}, "Material");

    MaterialComponent mat;
    std::string typeStr = node.GetChildValue("Type", "PHONG");

    mat.desc.opacity = std::stof(node.GetChildValue("Opacity", "1.0"));
    mat.desc.alphaCutoff = std::stof(node.GetChildValue("AlphaCutoff", "0.5"));

    if (!node.GetChildValue("UVScale").empty()) {
        std::stringstream ss(node.GetChildValue("UVScale"));
        ss >> mat.desc.uvScale.x >> mat.desc.uvScale.y;
    }
    if (!node.GetChildValue("UVOffset").empty()) {
        std::stringstream ss(node.GetChildValue("UVOffset"));
        ss >> mat.desc.uvOffset.x >> mat.desc.uvOffset.y;
    }

    auto parseBlend = [](const std::string &str, BlendFactor defaultFactor) -> BlendFactor
    {
        if (str == "Zero")
            return BlendFactor::Zero;
        if (str == "One")
            return BlendFactor::One;
        if (str == "SrcAlpha")
            return BlendFactor::SrcAlpha;
        if (str == "OneMinusSrcAlpha")
            return BlendFactor::OneMinusSrcAlpha;
        return defaultFactor;
    };

    mat.desc.blendSrc = parseBlend(node.GetChildValue("BlendSrc"), BlendFactor::SrcAlpha);
    mat.desc.blendDst = parseBlend(node.GetChildValue("BlendDst"), BlendFactor::OneMinusSrcAlpha);

    if (typeStr == "PBR")
    {
        mat.desc.type = MaterialType::PBR;
        mat.desc.roughness = std::stof(node.GetChildValue("Roughness", "0.5"));
        mat.desc.metallic = std::stof(node.GetChildValue("Metallic", "0.0"));
        mat.desc.ao = std::stof(node.GetChildValue("AO", "1.0"));

        std::stringstream emissSS(node.GetChildValue("Emission", "0 0 0"));
        float er, eg, eb;
        emissSS >> er >> eg >> eb;
        mat.desc.emission = glm::vec3(er, eg, eb);
    }
    else
    {
        mat.desc.type = MaterialType::PHONG;
        mat.desc.shininess = std::stof(node.GetChildValue("Shininess", "32.0"));

        std::stringstream specSS(node.GetChildValue("Specular", "0.5 0.5 0.5"));
        float sr = 0, sg = 0, sb = 0;
        specSS >> sr >> sg >> sb;
        mat.desc.specular = glm::vec3(sr, sg, sb);

        std::stringstream emissSS(node.GetChildValue("Emission", "0 0 0"));
        float er = 0, eg = 0, eb = 0;
        emissSS >> er >> eg >> eb;
        mat.desc.emission = glm::vec3(er, eg, eb);

        std::stringstream ambSS(node.GetChildValue("Ambient", "1 1 1"));
        float ar = 1, ag = 1, ab = 1;
        ambSS >> ar >> ag >> ab;
        mat.desc.ambient = glm::vec3(ar, ag, ab);
    }

    auto loadTex = [&](const std::string &key, std::string &outPath) {
        std::string path = node.GetChildValue(key);
        if (!path.empty()) {
            outPath = path;
        }
    };

    loadTex("Albedo", mat.desc.albedoPath);
    if (mat.desc.albedoPath.empty()) {
        loadTex("Diffuse", mat.desc.albedoPath);
    }
    loadTex("Normal", mat.desc.normalPath);
    loadTex("MetallicMap", mat.desc.metallicPath);
    loadTex("RoughnessMap", mat.desc.roughnessPath);
    loadTex("AOMap", mat.desc.aoPath);
    loadTex("EmissiveMap", mat.desc.emissivePath);

    mat.gpu.albedoMap = res.GetTextureAuto(mat.desc.albedoPath) ? res.GetTextureAuto(mat.desc.albedoPath)->id : 0;
    mat.gpu.normalMap = res.GetTextureAuto(mat.desc.normalPath) ? res.GetTextureAuto(mat.desc.normalPath)->id : 0;
    mat.gpu.metallicMap = res.GetTextureAuto(mat.desc.metallicPath) ? res.GetTextureAuto(mat.desc.metallicPath)->id : 0;
    mat.gpu.roughnessMap = res.GetTextureAuto(mat.desc.roughnessPath) ? res.GetTextureAuto(mat.desc.roughnessPath)->id : 0;
    mat.gpu.aoMap = res.GetTextureAuto(mat.desc.aoPath) ? res.GetTextureAuto(mat.desc.aoPath)->id : 0;
    mat.gpu.emissiveMap = res.GetTextureAuto(mat.desc.emissivePath) ? res.GetTextureAuto(mat.desc.emissivePath)->id : 0;

    mat.gpu.dirty = false;

    scene.registry.emplace<MaterialComponent>(entity, mat);
}

void ComponentLoader::LoadPathFollower(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"MoveSpeed", "RotationSpeed", "MaxRotationSpeed", "RotationAcceleration", "RotationOffset", "ArrivalDistance"}, "PathFollower");

    auto &pf = scene.registry.emplace<PathFollowerComponent>(entity);
    pf.moveSpeed = std::stof(node.GetChildValue("MoveSpeed", "5.0"));
    pf.rotationSpeed = std::stof(node.GetChildValue("RotationSpeed", "10.0"));
    pf.maxRotationSpeed = std::stof(node.GetChildValue("MaxRotationSpeed", "20.0"));
    pf.rotationAcceleration = std::stof(node.GetChildValue("RotationAcceleration", "40.0"));

    std::string ro = node.GetChildValue("RotationOffset", "0 0 0");
    std::stringstream roSS(ro);
    float rx = 0, ry = 0, rz = 0;
    if (roSS >> rx >> ry >> rz) {
        pf.rotationOffset = glm::vec3(rx, ry, rz);
    } else {

        try {
            pf.rotationOffset = glm::vec3(0, std::stof(ro), 0);
        } catch (...) {
            pf.rotationOffset = glm::vec3(0.0f);
        }
    }

    pf.arrivalDistance = std::stof(node.GetChildValue("ArrivalDistance", "0.5"));
}

void ComponentLoader::LoadDecal(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    LoaderUtils::ValidateKeys(node, {"Albedo", "Normal", "Opacity", "Lifetime", "TargetTags"}, "Decal");

    auto &d = scene.registry.emplace<DecalComponent>(entity);
    
    auto getOrLoadTex = [&](const std::string& path) -> std::shared_ptr<Texture> {
        return res.GetTextureAuto(path);
    };

    std::string albedoPath = node.GetChildValue("Albedo");
    auto albedoTex = getOrLoadTex(albedoPath);
    if (albedoTex) d.albedoMap = albedoTex->id;

    std::string normalPath = node.GetChildValue("Normal");
    auto normalTex = getOrLoadTex(normalPath);
    if (normalTex) d.normalMap = normalTex->id;

    d.opacity = std::stof(node.GetChildValue("Opacity", "1.0"));
    d.lifetime = std::stof(node.GetChildValue("Lifetime", "-1.0"));

    std::string tags = node.GetChildValue("TargetTags");
    if (!tags.empty())
    {
        std::stringstream ss(tags);
        std::string tag;
        while (ss >> tag)
        {
            d.targetTags.push_back(tag);
        }
    }
}