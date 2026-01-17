#include <scene/component_loader.h>
#include <script/script_registry.h>
#include <utils/filesystem.h>
#include <iostream>

void ComponentLoader::LoadRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    std::string modelName, shaderName;
    ss >> modelName >> shaderName;
    auto &r = scene.registry.emplace<MeshRendererComponent>(entity);
    r.model = res.GetModel(modelName);
    r.shader = res.GetShader(shaderName);
}

void ComponentLoader::LoadAnimator(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    std::string animName;
    ss >> animName;

    float speed = 1.0f;
    float startTime = 0.0f;
    float rate = 30.0f;

    if (ss >> speed)
    {
        if (ss >> startTime)
        {
            ss >> rate;
        }
    }

    auto &a = scene.registry.emplace<AnimationComponent>(entity);

    a.animator = new Animator(res.GetAnimation(animName));
    a.animator->SetSpeed(speed);
    a.animator->SetTime(startTime);
    a.animator->SetUpdateRate(rate);
}

void ComponentLoader::LoadLightDir(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    float dx, dy, dz, r, g, b, i;
    ss >> dx >> dy >> dz >> r >> g >> b >> i;
    auto &l = scene.registry.emplace<DirectionalLightComponent>(entity);
    l.direction = glm::vec3(dx, dy, dz);
    l.color = glm::vec3(r, g, b);
    l.intensity = i;

    float ambientStr = 0.2f;
    float diffuseStr = 0.8f;

    if (ss >> ambientStr)
    {
        if (ss >> diffuseStr)
        {
        }
    }

    l.ambient = l.color * ambientStr;
    l.diffuse = l.color * diffuseStr;
    l.specular = glm::vec3(0.5f);
}

void ComponentLoader::LoadLightPoint(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    float r, g, b, i, rad;
    ss >> r >> g >> b >> i >> rad;
    auto &l = scene.registry.emplace<PointLightComponent>(entity);
    l.color = glm::vec3(r, g, b);
    l.intensity = i;
    l.radius = rad;

    float c, lin, quad;
    if (ss >> c >> lin >> quad)
    {
        l.constant = c;
        l.linear = lin;
        l.quadratic = quad;
    }

    float ambStr = 0.1f;
    float diffStr = 1.0f;
    if (ss >> ambStr)
    {
        if (ss >> diffStr)
        {
        }
    }
    l.ambient = l.color * ambStr;
    l.diffuse = l.color * diffStr;
    l.specular = glm::vec3(1.0f);
}

void ComponentLoader::LoadLightSpot(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    float r, g, b, i, cut, outer;
    ss >> r >> g >> b >> i >> cut >> outer;
    auto &l = scene.registry.emplace<SpotLightComponent>(entity);
    l.color = glm::vec3(r, g, b);
    l.intensity = i;
    l.cutOff = glm::cos(glm::radians(cut));
    l.outerCutOff = glm::cos(glm::radians(outer));

    float c, lin, quad;
    if (ss >> c >> lin >> quad)
    {
        l.constant = c;
        l.linear = lin;
        l.quadratic = quad;
    }

    float ambStr = 0.1f;
    float diffStr = 1.0f;
    if (ss >> ambStr)
    {
        if (ss >> diffStr)
        {
        }
    }
    l.ambient = l.color * ambStr;
    l.diffuse = l.color * diffStr;
    l.specular = glm::vec3(1.0f);
}

void ComponentLoader::LoadUITransform(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    float x, y, w, h;
    int z;
    ss >> x >> y >> w >> h >> z;
    scene.registry.emplace<UITransformComponent>(entity, glm::vec2(x, y), glm::vec2(w, h), z);
}

void ComponentLoader::LoadUIRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    float r, g, b, a;
    std::string shaderName;
    ss >> r >> g >> b >> a >> shaderName;
    auto &ui = scene.registry.emplace<UIRendererComponent>(entity);
    ui.color = glm::vec4(r, g, b, a);
    ui.shader = res.GetShader(shaderName);

    if (!res.GetUIModel("default_rect"))
        res.CreateUIModel("default_rect", UIType::Color);
    ui.model = res.GetUIModel("default_rect");
}

void ComponentLoader::LoadUIText(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    std::string textContent;
    std::string fontName;
    float r, g, b, scale;

    ss >> std::ws;
    if (ss.peek() == '"')
    {
        char quote;
        ss >> quote;
        std::getline(ss, textContent, '"');
    }
    else
    {
        ss >> textContent;
    }

    ss >> fontName >> r >> g >> b >> scale;

    auto &txt = scene.registry.emplace<UITextComponent>(entity);
    txt.text = textContent;
    txt.font = res.GetFont(fontName);
    txt.color = glm::vec3(r, g, b);
    txt.scale = scale;

    if (!res.GetUIModel("default_text_rect"))
        res.CreateUIModel("default_text_rect", UIType::Text);
    txt.model = res.GetUIModel("default_text_rect");
    txt.shader = res.GetShader("textShader");
}

void ComponentLoader::LoadUIAnimation(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    float hr, hg, hb, ha;
    ss >> hr >> hg >> hb >> ha;
    auto &anim = scene.registry.emplace<UIAnimationComponent>(entity);
    anim.hoverColor = glm::vec4(hr, hg, hb, ha);

    auto &ui = scene.registry.get<UIRendererComponent>(entity);
    anim.normalColor = ui.color;
}

void ComponentLoader::LoadSkyboxRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    std::string skyboxName, shaderName;
    ss >> skyboxName >> shaderName;

    auto &comp = scene.registry.emplace<SkyboxRenderComponent>(entity);
    comp.skybox = res.GetSkybox(skyboxName);
    comp.shader = res.GetShader(shaderName);
}

void ComponentLoader::LoadScript(Scene& scene, entt::entity entity, std::stringstream& ss, Application* app)
{
    std::string className;
    ss >> className;

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

void ComponentLoader::LoadAudioSource(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    std::string path;
    float vol, minDur;
    int loop, is3d, awake;
    ss >> path >> vol >> loop >> is3d >> minDur >> awake;

    AudioSourceComponent audio;
    audio.filePath = path;
    audio.volume = vol;
    audio.loop = (loop != 0);
    audio.is3D = (is3d != 0);
    audio.minDistance = minDur;
    audio.playOnAwake = (awake != 0);

    scene.registry.emplace<AudioSourceComponent>(entity, audio);
}

void ComponentLoader::LoadVideoPlayer(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    std::string path;
    int loop = 0, playOnAwake = 1;
    float speed = 1.0f;
    ss >> path;
    
    if (!ss.eof()) ss >> loop;
    if (!ss.eof()) ss >> speed;
    if (!ss.eof()) ss >> playOnAwake;

    VideoPlayerComponent video;
    video.filePath = FileSystem::getPath(path);
    video.isLooping = (loop != 0);
    video.speed = speed;
    video.playOnAwake = (playOnAwake != 0);

    scene.registry.emplace<VideoPlayerComponent>(entity, video);
}

void ComponentLoader::LoadParticleEmitter(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res)
{
    std::string texName;
    int maxParticles;
    float life;
    ss >> texName >> maxParticles >> life;

    auto &emitterComp = scene.registry.emplace<ParticleEmitterComponent>(entity);
    emitterComp.emitter.Init(maxParticles);
    emitterComp.emitter.LifeTime = life;
    emitterComp.emitter.StartLife = life;

    emitterComp.emitter.texture = res.GetTexture(texName);

    if (!emitterComp.emitter.texture)
    {
        std::cerr << "[ComponentLoader] Particle Texture not found: " << texName << std::endl;
    }
}

void ComponentLoader::LoadMaterial(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    std::string typeStr;
    ss >> typeStr;

    MaterialComponent mat;

    if (typeStr == "PBR")
    {
        mat.type = MaterialType::PBR;
        ss >> mat.roughness >> mat.metallic >> mat.ao;

        float er = 0.0f, eg = 0.0f, eb = 0.0f;
        if (ss >> er >> eg >> eb)
        {
            mat.emission = glm::vec3(er, eg, eb);
        }
    }
    else if (typeStr == "PHONG")
    {
        mat.type = MaterialType::PHONG;
        float r = 0.5f, g = 0.5f, b = 0.5f;
        ss >> mat.shininess >> r >> g >> b;
        mat.specular = glm::vec3(r, g, b);

        float er = 0.0f, eg = 0.0f, eb = 0.0f;
        if (ss >> er >> eg >> eb)
        {
            mat.emission = glm::vec3(er, eg, eb);
        }

        float ar = 1.0f, ag = 1.0f, ab = 1.0f;
        if (ss >> ar >> ag >> ab)
        {
            mat.ambient = glm::vec3(ar, ag, ab);
        }
    }
    else
    {
        try
        {
            mat.type = MaterialType::PHONG;
            mat.shininess = std::stof(typeStr);

            float r = 0.5f, g = 0.5f, b = 0.5f;
            if (ss >> r >> g >> b)
            {
                mat.specular = glm::vec3(r, g, b);
            }
        }
        catch (...)
        {
            std::cout << "Invalid MATERIAL format for entity " << (int)entity << std::endl;
        }
    }

    scene.registry.emplace<MaterialComponent>(entity, mat);
}

void ComponentLoader::LoadCamera(Scene& scene, entt::entity entity, std::stringstream& ss)
{
    int isPrimary;
    float fov, yaw, pitch;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    ss >> isPrimary >> fov >> yaw >> pitch;

    if (!ss.eof())
        ss >> nearPlane;
    if (!ss.eof())
        ss >> farPlane;

    auto &c = scene.registry.emplace<CameraComponent>(entity);
    c.isPrimary = (bool)isPrimary;
    c.fov = fov;
    c.yaw = yaw;
    c.pitch = pitch;
    c.nearPlane = nearPlane;
    c.farPlane = farPlane;
}
