#include <ecs/entity_builder.h>
#include <app/application.h>
#include <resource/resource_manager.h>
#include <ecs/components/info_component.h>
#include <graphic/geometry/animator.h>

EntityBuilder::EntityBuilder(Scene& scene, Application* app)
    : m_Scene(scene), m_App(app)
{
    m_Entity = m_Scene.registry.create();
}

EntityBuilder& EntityBuilder::WithName(const std::string& name)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.name = name;
    return *this;
}

EntityBuilder& EntityBuilder::WithTag(const std::string& tag)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.tag = tag;
    return *this;
}

EntityBuilder& EntityBuilder::WithLayer(uint32_t layer)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.layer = layer;
    return *this;
}

EntityBuilder& EntityBuilder::WithTransform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
{
    auto& transform = m_Scene.registry.get_or_emplace<TransformComponent>(m_Entity);
    transform.position = pos;
    transform.rotation = rot; 
    transform.scale = scale;
    return *this;
}

EntityBuilder& EntityBuilder::WithMesh(const std::string& modelName, const std::string& shaderName)
{
    auto& res = m_App->GetResourceManager();
    auto& mesh = m_Scene.registry.get_or_emplace<MeshRendererComponent>(m_Entity);
    mesh.model = res.GetModel(modelName);
    mesh.shader = res.GetShader(shaderName);
    return *this;
}

EntityBuilder& EntityBuilder::WithMaterial(const MaterialComponent& material)
{
    m_Scene.registry.emplace_or_replace<MaterialComponent>(m_Entity, material);
    return *this;
}

EntityBuilder& EntityBuilder::WithPhongMaterial(const glm::vec3& ambient, const glm::vec3& specular, float shininess)
{
    auto& mat = m_Scene.registry.get_or_emplace<MaterialComponent>(m_Entity);
    mat.type = MaterialType::PHONG;
    mat.ambient = ambient;
    mat.specular = specular;
    mat.shininess = shininess;
    return *this;
}

EntityBuilder& EntityBuilder::WithPBRMaterial(float metallic, float roughness, float ao)
{
    auto& mat = m_Scene.registry.get_or_emplace<MaterialComponent>(m_Entity);
    mat.type = MaterialType::PBR;
    mat.metallic = metallic;
    mat.roughness = roughness;
    mat.ao = ao;
    return *this;
}

EntityBuilder& EntityBuilder::WithRigidBody(std::shared_ptr<IRigidBody> body)
{
    auto& rb = m_Scene.registry.get_or_emplace<RigidBodyComponent>(m_Entity);
    rb.body = body;
    return *this;
}

EntityBuilder& EntityBuilder::WithUITransform(const glm::vec2& pos, const glm::vec2& size, int zIndex)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    uiTransform.position = pos;
    uiTransform.size = size;
    uiTransform.zIndex = zIndex;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIText(const std::string& text, const std::string& fontName, float scale, const glm::vec3& color)
{
    auto& res = m_App->GetResourceManager();
    auto& textComp = m_Scene.registry.get_or_emplace<UITextComponent>(m_Entity);
    
    textComp.text = text;
    textComp.fontName = fontName;
    textComp.scale = scale;
    textComp.color = color;
    
    textComp.font = res.GetFont(fontName);
    textComp.shader = res.GetShader("ui_text");
    
    std::string uniqueModelName = "ui_text_model_" + std::to_string((uint32_t)m_Entity);
    if (!res.GetUIModel(uniqueModelName))
    {
        res.CreateUIModel(uniqueModelName, UIType::Text);
    }
    textComp.model = res.GetUIModel(uniqueModelName);
    
    return *this;
}

EntityBuilder& EntityBuilder::WithAudio(const std::string& soundName, bool loop, float volume)
{
    auto& res = m_App->GetResourceManager();
    auto& audio = m_Scene.registry.get_or_emplace<AudioSourceComponent>(m_Entity);
    audio.sound = std::dynamic_pointer_cast<ISound>(res.GetSound(soundName));
    if (audio.sound)
    {
        audio.loop = loop;
        audio.volume = volume;
    }
    return *this;
}

EntityBuilder& EntityBuilder::WithScript(const std::string& scriptName)
{
    auto& script = m_Scene.registry.get_or_emplace<ScriptComponent>(m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithAnimation(const std::string& animationName)
{
    auto& res = m_App->GetResourceManager();
    auto& anim = m_Scene.registry.get_or_emplace<AnimationComponent>(m_Entity);
    anim.animations.push_back(animationName);
    
    auto a = res.GetAnimation(animationName);
    if (a)
    {
        if (!anim.animator)
        {
            anim.animator = std::make_shared<Animator>(a);
        }
        else
        {
            anim.animator->AddAnimation(animationName, a);
        }
    }
    return *this;
}

EntityBuilder& EntityBuilder::WithDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    auto& light = m_Scene.registry.get_or_emplace<DirectionalLightComponent>(m_Entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    return *this;
}

EntityBuilder& EntityBuilder::WithPointLight(const glm::vec3& color, float intensity, float radius)
{
    auto& light = m_Scene.registry.get_or_emplace<PointLightComponent>(m_Entity);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    return *this;
}

EntityBuilder& EntityBuilder::WithSpotLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    auto& light = m_Scene.registry.get_or_emplace<SpotLightComponent>(m_Entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    return *this;
}

EntityBuilder& EntityBuilder::WithCamera(float fov, float near, float far, bool active)
{
    auto& cam = m_Scene.registry.get_or_emplace<CameraComponent>(m_Entity);
    cam.fov = fov;
    cam.nearPlane = near;
    cam.farPlane = far;
    if (active)
        m_Scene.SetActiveCamera(m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithParticle(const std::string& textureName)
{
    auto& p = m_Scene.registry.get_or_emplace<ParticleEmitterComponent>(m_Entity);
    p.textureName = textureName;
    return *this;
}

EntityBuilder& EntityBuilder::WithVideo(const std::string& videoPath, bool loop)
{
    auto& video = m_Scene.registry.get_or_emplace<VideoPlayerComponent>(m_Entity);
    video.filePath = videoPath;
    video.isLooping = loop;
    return *this;
}

entt::entity EntityBuilder::Build()
{
    return m_Entity;
}
