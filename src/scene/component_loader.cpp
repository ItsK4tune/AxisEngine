#include <utils/filesystem.h>
#include <scene/component_loader.h>
#include <utils/logger.h>
#include <script/script_registry.h>
#include <iostream>
#include <algorithm>

void ComponentLoader::ValidateKeys(const YAMLNode &node, const std::vector<std::string> &allowedKeys, const std::string &componentName)
{
    for (const auto &child : node.children)
    {
        if (std::find(allowedKeys.begin(), allowedKeys.end(), child.key) == allowedKeys.end())
        {
            LOGGER_WARN("ComponentLoader") << "Unknown key '" << child.key << "' in component '" << componentName << "'";
        }
    }
}

void ComponentLoader::LoadRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Model", "Shader", "Order", "Color", "CastShadow"}, "Renderer");

    std::string modelName = node.GetChildValue("Model");
    std::string shaderName = node.GetChildValue("Shader");
    int order = std::stoi(node.GetChildValue("Order", "0"));
    bool castShadow = node.GetChildValue("CastShadow", "1") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1 1"));
    float cr, cg, cb, ca;
    colorSS >> cr >> cg >> cb >> ca;
    glm::vec4 color(cr, cg, cb, ca);

    if (modelName.empty() || shaderName.empty())
    {
        LOGGER_WARN("ComponentLoader") << "Renderer component missing 'Model' or 'Shader' field";
    }

    auto &r = scene.registry.emplace<MeshRendererComponent>(entity);
    r.model = res.GetModel(modelName);
    r.shader = res.GetShader(shaderName);
    r.order = order;
    r.castShadow = castShadow;
    r.color = color;

    if (!r.model)
        LOGGER_WARN("ComponentLoader") << "Renderer model not found: " << modelName;
    if (r.shader.expired())
        LOGGER_WARN("ComponentLoader") << "Renderer shader not found: " << shaderName;
}

void ComponentLoader::LoadAnimator(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Animation", "Speed", "StartTime", "Rate"}, "Animator");

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

    if (a.speed < 0.0f)
        LOGGER_WARN("ComponentLoader") << "Animator Speed should not be negative: " << a.speed;
    if (a.rate <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "Animator Rate should be positive: " << a.rate;

    if (!a.animations.empty())
    {
        auto firstAnim = res.GetAnimation(a.animations[0]);
        if (firstAnim)
        {
            a.animator = std::make_shared<Animator>(firstAnim);
            a.animator->AddAnimation(a.animations[0], firstAnim);
            a.animator->SetSpeed(a.speed);
            a.animator->SetTime(a.startTime);
            a.animator->SetUpdateRate(a.rate);

            if (scene.registry.all_of<MeshRendererComponent>(entity))
            {
                auto& mrc = scene.registry.get<MeshRendererComponent>(entity);
                if (mrc.model && mrc.model->IsStatic())
                {
                    LOGGER_WARN("ComponentLoader") << "Entity has Animator but its Model is STATIC! Animations will not play correctly. Entity: " 
                                                   << (scene.registry.all_of<InfoComponent>(entity) ? scene.registry.get<InfoComponent>(entity).name : "Unknown");
                }
            }

            for (size_t i = 1; i < a.animations.size(); ++i)
            {
                auto extraAnim = res.GetAnimation(a.animations[i]);
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
    ValidateKeys(node, {"Primary", "FOV", "Yaw", "Pitch", "Near", "Far"}, "Camera");

    auto &c = scene.registry.emplace<CameraComponent>(entity);
    c.isPrimary = node.GetChildValue("Primary", "1") == "1" || node.GetChildValue("Primary", "true") == "true";

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
        scene.SetActiveCamera(entity);
}

void ComponentLoader::LoadLightDir(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "AmbientStr", "DiffuseStr", "SpecularStr"}, "LightDir");

    auto &l = scene.registry.emplace<DirectionalLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r, g, b;
    colorSS >> r >> g >> b;
    l.color = glm::vec3(r, g, b);

    l.intensity = std::stof(node.GetChildValue("Intensity", "1.0"));
    if (l.intensity < 0.0f)
        LOGGER_WARN("ComponentLoader") << "LightDir Intensity should not be negative: " << l.intensity;

    float ambientStr = std::stof(node.GetChildValue("AmbientStr", "0.2"));
    float diffuseStr = std::stof(node.GetChildValue("DiffuseStr", "0.8"));
    float specularStr = std::stof(node.GetChildValue("SpecularStr", "0.5"));

    l.ambient = l.color * ambientStr;
    l.diffuse = l.color * diffuseStr;
    l.specular = l.color * specularStr;
}

void ComponentLoader::LoadLightPoint(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "Radius", "Constant", "Linear", "Quadratic", "AmbientStr", "DiffuseStr", "SpecularStr"}, "LightPoint");

    auto &l = scene.registry.emplace<PointLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r, g, b;
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

    float ambStr = std::stof(node.GetChildValue("AmbientStr", "0.1"));
    float diffStr = std::stof(node.GetChildValue("DiffuseStr", "1.0"));
    float specStr = std::stof(node.GetChildValue("SpecularStr", "1.0"));

    l.ambient = l.color * ambStr;
    l.diffuse = l.color * diffStr;
    l.specular = l.color * specStr;
}

void ComponentLoader::LoadLightSpot(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Active", "CastShadow", "Color", "Intensity", "CutOff", "OuterCutOff", "Constant", "Linear", "Quadratic", "AmbientStr", "DiffuseStr"}, "LightSpot");

    auto &l = scene.registry.emplace<SpotLightComponent>(entity);

    l.active = node.GetChildValue("Active", "1") == "1" || node.GetChildValue("Active", "true") == "true";
    l.isCastShadow = node.GetChildValue("CastShadow", "0") == "1" || node.GetChildValue("CastShadow", "true") == "true";

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r, g, b;
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

    float ambStr = std::stof(node.GetChildValue("AmbientStr", "0.1"));
    float diffStr = std::stof(node.GetChildValue("DiffuseStr", "1.0"));

    l.ambient = l.color * ambStr;
    l.diffuse = l.color * diffStr;
    l.specular = glm::vec3(1.0f);
}

void ComponentLoader::LoadUITransform(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Position", "Size", "ZOrder", "UsePercentage", "Anchor"}, "UITransform");

    auto &ui = scene.registry.emplace<UITransformComponent>(entity);

    std::stringstream posSS(node.GetChildValue("Position", "0 0"));
    float x, y;
    posSS >> x >> y;
    ui.position = glm::vec2(x, y);

    std::stringstream sizeSS(node.GetChildValue("Size", "100 100"));
    float w, h;
    sizeSS >> w >> h;
    ui.size = glm::vec2(w, h);

    if (w < 0.0f || h < 0.0f)
        LOGGER_WARN("ComponentLoader") << "UITransform Size should not be negative: " << w << "x" << h;

    ui.zIndex = std::stoi(node.GetChildValue("ZOrder", "0"));
    ui.usePercentage = node.GetChildValue("UsePercentage", "0") == "1" || node.GetChildValue("UsePercentage", "true") == "true";

    std::stringstream anchorSS(node.GetChildValue("Anchor", "0 0"));
    float ax, ay;
    anchorSS >> ax >> ay;
    ui.anchor = glm::vec2(ax, ay);
}

void ComponentLoader::LoadUIRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Color", "Shader"}, "UIRenderer");

    auto &ui = scene.registry.emplace<UIRendererComponent>(entity);

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1 1"));
    float r, g, b, a;
    colorSS >> r >> g >> b >> a;
    ui.color = glm::vec4(r, g, b, a);

    std::string shaderName = node.GetChildValue("Shader");
    ui.shader = res.GetShader(shaderName);
    if (!ui.shader)
        LOGGER_WARN("ComponentLoader") << "UIRenderer shader not found: " << shaderName;

    if (!res.GetUIModel("default_rect"))
        res.CreateUIModel("default_rect", UIType::Color);
    ui.model = res.GetUIModel("default_rect");
}

void ComponentLoader::LoadUIText(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Text", "Font", "Color", "Scale"}, "UIText");

    auto &txt = scene.registry.emplace<UITextComponent>(entity);

    std::string textContent = node.GetChildValue("Text");
    if (textContent.length() > 2 && textContent.front() == '"' && textContent.back() == '"')
    {
        textContent = textContent.substr(1, textContent.length() - 2);
    }
    txt.text = textContent;

    std::string fontName = node.GetChildValue("Font");
    txt.font = res.GetFont(fontName);
    if (!txt.font)
        LOGGER_WARN("ComponentLoader") << "UIText font not found: " << fontName;

    std::stringstream colorSS(node.GetChildValue("Color", "1 1 1"));
    float r, g, b;
    colorSS >> r >> g >> b;
    txt.color = glm::vec3(r, g, b);

    txt.scale = std::stof(node.GetChildValue("Scale", "1.0"));

    if (!res.GetUIModel("default_text_rect"))
        res.CreateUIModel("default_text_rect", UIType::Text);
    txt.model = res.GetUIModel("default_text_rect");
    txt.shader = res.GetShader("textShader");
}

void ComponentLoader::LoadLOD(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Models", "Distances"}, "LOD");

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
        auto model = res.GetModel(modelName);
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
    ValidateKeys(node, {"Skybox", "Shader"}, "SkyboxRenderer");

    auto &comp = scene.registry.emplace<SkyboxRenderComponent>(entity);

    std::string skyboxName = node.GetChildValue("Skybox");
    std::string shaderName = node.GetChildValue("Shader");

    comp.skybox = res.GetSkybox(skyboxName);
    comp.shader = res.GetShader(shaderName);

    if (!comp.skybox)
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer skybox not found: " << skyboxName;
    if (comp.shader.expired())
        LOGGER_WARN("ComponentLoader") << "SkyboxRenderer shader not found: " << shaderName;
    scene.SetActiveSkybox(entity);
}

void ComponentLoader::LoadScript(Scene &scene, entt::entity entity, const YAMLNode &node, Application *app)
{
    ValidateKeys(node, {"Class"}, "Script");

    std::string className = node.GetChildValue("Class");
    if (className.empty())
        LOGGER_WARN("ComponentLoader") << "Script component missing 'Class' property";

    auto &scriptComp = scene.registry.emplace<ScriptComponent>(entity);
    Scriptable *scriptInstance = ScriptRegistry::Instance().Create(className);

    if (scriptInstance)
    {
        scriptComp.instance = scriptInstance;
        scriptComp.InstantiateScript = [className]()
        { return ScriptRegistry::Instance().Create(className); };
        scriptComp.DestroyScript = [](ScriptComponent *nsc)
        { delete nsc->instance; nsc->instance = nullptr; };
        scriptComp.instance->Init(entity, &scene, app);
        scriptComp.instance->OnCreate();
    }
}

void ComponentLoader::LoadAudioSource(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Path", "Volume", "Loop", "Is3D", "MinDistance", "PlayOnAwake"}, "AudioSource");

    AudioSourceComponent audio;
    audio.filePath = node.GetChildValue("Path");
    if (audio.filePath.empty())
        LOGGER_WARN("ComponentLoader") << "AudioSource missing 'Path' property";

    audio.volume = std::stof(node.GetChildValue("Volume", "1.0"));
    if (audio.volume < 0.0f || audio.volume > 1.0f)
        LOGGER_WARN("ComponentLoader") << "AudioSource Volume out of bounds (0-1): " << audio.volume;

    audio.loop = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";
    audio.is3D = node.GetChildValue("Is3D", "0") == "1" || node.GetChildValue("Is3D", "true") == "true";

    audio.minDistance = std::stof(node.GetChildValue("MinDistance", "1.0"));
    if (audio.minDistance <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "AudioSource MinDistance must be > 0: " << audio.minDistance;
    audio.playOnAwake = node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";

    scene.registry.emplace<AudioSourceComponent>(entity, audio);
}

void ComponentLoader::LoadVideoPlayer(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    ValidateKeys(node, {"Path", "Loop", "Speed", "PlayOnAwake"}, "VideoPlayer");

    VideoPlayerComponent video;
    video.filePath = FileSystem::getPath(node.GetChildValue("Path"));
    video.isLooping = node.GetChildValue("Loop", "0") == "1" || node.GetChildValue("Loop", "true") == "true";

    video.speed = std::stof(node.GetChildValue("Speed", "1.0"));
    if (video.speed < 0.0f)
        LOGGER_WARN("ComponentLoader") << "VideoPlayer Speed must be positive: " << video.speed;
    video.playOnAwake = node.GetChildValue("PlayOnAwake", "1") == "1" || node.GetChildValue("PlayOnAwake", "true") == "true";

    scene.registry.emplace<VideoPlayerComponent>(entity, video);
}

void ComponentLoader::LoadParticleEmitter(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Texture", "MaxParticles", "Life"}, "ParticleEmitter");

    std::string texName = node.GetChildValue("Texture");

    int maxParticles = std::stoi(node.GetChildValue("MaxParticles", "100"));
    if (maxParticles <= 0)
        LOGGER_WARN("ComponentLoader") << "ParticleEmitter MaxParticles must be > 0: " << maxParticles;

    float life = std::stof(node.GetChildValue("Life", "1.0"));
    if (life <= 0.0f)
        LOGGER_WARN("ComponentLoader") << "ParticleEmitter Life must be > 0: " << life;

    auto &emitterComp = scene.registry.emplace<ParticleEmitterComponent>(entity);
    emitterComp.emitter.Init(maxParticles);
    emitterComp.emitter.LifeTime = life;
    emitterComp.emitter.StartLife = life;

    emitterComp.emitter.texture = res.GetTexture(texName);

    if (!emitterComp.emitter.texture)
    {
        LOGGER_ERROR("ComponentLoader") << "Particle Texture not found: " << texName;
    }
}

void ComponentLoader::LoadMaterial(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res)
{
    ValidateKeys(node, {"Type", "Roughness", "Metallic", "AO", "Shininess", "Specular", "Emission", "Ambient", "Opacity", "AlphaCutoff", "BlendSrc", "BlendDst", "Albedo", "Diffuse", "Normal", "MetallicMap", "RoughnessMap", "AOMap", "EmissiveMap"}, "Material");

    MaterialComponent mat;
    std::string typeStr = node.GetChildValue("Type", "PHONG");

    mat.opacity = std::stof(node.GetChildValue("Opacity", "1.0"));
    mat.alphaCutoff = std::stof(node.GetChildValue("AlphaCutoff", "0.5"));

    auto parseBlend = [](const std::string& str, Graphics::BlendFactor defaultFactor) -> Graphics::BlendFactor {
        if (str == "Zero") return Graphics::BlendFactor::Zero;
        if (str == "One") return Graphics::BlendFactor::One;
        if (str == "SrcAlpha") return Graphics::BlendFactor::SrcAlpha;
        if (str == "OneMinusSrcAlpha") return Graphics::BlendFactor::OneMinusSrcAlpha;
        // Add more if needed, default to provided default
        return defaultFactor;
    };

    mat.blendSrc = parseBlend(node.GetChildValue("BlendSrc", ""), Graphics::BlendFactor::SrcAlpha);
    mat.blendDst = parseBlend(node.GetChildValue("BlendDst", ""), Graphics::BlendFactor::OneMinusSrcAlpha);

    if (typeStr == "PBR")
    {
        mat.type = MaterialType::PBR;
        mat.roughness = std::stof(node.GetChildValue("Roughness", "0.5"));
        mat.metallic = std::stof(node.GetChildValue("Metallic", "0.0"));
        mat.ao = std::stof(node.GetChildValue("AO", "1.0"));

        std::stringstream emissSS(node.GetChildValue("Emission", "0 0 0"));
        float er, eg, eb;
        emissSS >> er >> eg >> eb;
        mat.emission = glm::vec3(er, eg, eb);
    }
    else
    {
        mat.type = MaterialType::PHONG;
        mat.shininess = std::stof(node.GetChildValue("Shininess", "32.0"));

        std::stringstream specSS(node.GetChildValue("Specular", "0.5 0.5 0.5"));
        float sr, sg, sb;
        specSS >> sr >> sg >> sb;
        mat.specular = glm::vec3(sr, sg, sb);

        std::stringstream emissSS(node.GetChildValue("Emission", "0 0 0"));
        float er, eg, eb;
        emissSS >> er >> eg >> eb;
        mat.emission = glm::vec3(er, eg, eb);

        std::stringstream ambSS(node.GetChildValue("Ambient", "1 1 1"));
        float ar, ag, ab;
        ambSS >> ar >> ag >> ab;
        mat.ambient = glm::vec3(ar, ag, ab);
    }

    // Load textures
    auto loadTex = [&](const std::string& key, std::string& outPath, uint32_t& outID) {
        std::string path = node.GetChildValue(key);
        if (!path.empty()) {
            outPath = path;
            res.LoadTexture(path, path, false); // Load synchronously for material
            auto tex = res.GetTexture(path);
            if (tex) outID = tex->id;
        }
    };

    loadTex("Albedo", mat.albedoPath, mat.albedoMap);
    if (mat.albedoPath.empty()) {
        loadTex("Diffuse", mat.albedoPath, mat.albedoMap);
    }
    loadTex("Normal", mat.normalPath, mat.normalMap);
    loadTex("MetallicMap", mat.metallicPath, mat.metallicMap);
    loadTex("RoughnessMap", mat.roughnessPath, mat.roughnessMap);
    loadTex("AOMap", mat.aoPath, mat.aoMap);
    loadTex("EmissiveMap", mat.emissivePath, mat.emissiveMap);

    scene.registry.emplace<MaterialComponent>(entity, mat);
}
