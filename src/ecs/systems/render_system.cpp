#include <ecs/system.h>
#include <graphic/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <resource/resource_manager.h>

void RenderSystem::InitShadows(ResourceManager &res)
{
    m_Shadow.Init();

    glGenBuffers(1, &m_DirLightSSBO);
    glGenBuffers(1, &m_PointLightSSBO);
    glGenBuffers(1, &m_SpotLightSSBO);

    res.LoadShader("shadow_depth", "src/asset/shaders/shadow_depth.vs", "src/asset/shaders/shadow_depth.fs");
    res.LoadShader("shadow_point", "src/asset/shaders/shadow_point.vs", "src/asset/shaders/shadow_point.fs", "src/asset/shaders/shadow_point.gs");

    m_Shadow.SetShaderDir(res.GetShader("shadow_depth"));
    m_Shadow.SetShaderPoint(res.GetShader("shadow_point"));
}

void RenderSystem::Shutdown()
{
    m_Shadow.Shutdown();
}

void RenderSystem::RenderShadows(Scene &scene)
{
    // Mode 0: No shadows
    if (m_ShadowMode == 0)
        return;

    if (!m_EnableShadows)
        return;

    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();

    if (!shaderDir)
        return;

    // Collect directional lights with isCastShadow enabled
    std::vector<entt::entity> shadowCastingLights;
    auto dirLightView = scene.registry.view<DirectionalLightComponent>();
    
    for (auto entity : dirLightView)
    {
        auto& light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            shadowCastingLights.push_back(entity);
        }
    }
    
    // No shadow casting lights found
    if (shadowCastingLights.empty())
        return;

    // Mode 1: Render shadow from first light only
    int numShadowsToRender = 1;
    if (m_ShadowMode == 2)
    {
        // Mode 2: Render shadows from all lights (max 4)
        numShadowsToRender = std::min((int)shadowCastingLights.size(), 4);
        
        // Warning if more than 4 lights have isCastShadow
        if (shadowCastingLights.size() > 4)
        {
            std::cout << "[RenderSystem] More than 4 lights have isCastShadow enabled. Only the first 4 will cast shadows." << std::endl;
        }
    }

    // Get camera position for distance culling
    glm::vec3 camPos(0.0f);
    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto& camTrans = scene.registry.get<TransformComponent>(camEntity);
        camPos = camTrans.position;
    }

    // Render shadow maps for each shadow-casting light
    for (int lightIdx = 0; lightIdx < numShadowsToRender; ++lightIdx)
    {
        entt::entity lightEntity = shadowCastingLights[lightIdx];
        auto& light = scene.registry.get<DirectionalLightComponent>(lightEntity);
        
        // Calculate light direction
        glm::vec3 lightDir = light.direction;
        if (scene.registry.all_of<TransformComponent>(lightEntity))
        {
            auto& trans = scene.registry.get<TransformComponent>(lightEntity);
            lightDir = trans.rotation * glm::vec3(0, 0, -1);
        }
        lightDir = glm::normalize(lightDir);
        glm::vec3 lightPos = -lightDir * m_ShadowProjectionSize;

        // Calculate light space matrix
        glm::mat4 lightProjection = glm::ortho(-m_ShadowProjectionSize, m_ShadowProjectionSize, 
                                                -m_ShadowProjectionSize, m_ShadowProjectionSize, 
                                                0.1f, 200.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_LightSpaceMatrixDir[lightIdx] = lightProjection * lightView;

        // Frustum culling setup
        Frustum lightFrustum;
        if (m_ShadowFrustumCullingEnabled)
        {
            lightFrustum.Update(m_LightSpaceMatrixDir[lightIdx]);
        }

        // Bind shadow FBO and clear
        m_Shadow.BindFBO_Dir(lightIdx);
        glClear(GL_DEPTH_BUFFER_BIT);

        shaderDir->use();
        shaderDir->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir[lightIdx]);

        // Render all shadow-casting objects
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view)
        {
            auto [trans, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);
            
            // Skip if invalid model or shadow casting disabled
            if (!renderer.model || !renderer.castShadow)
                continue;

            // Distance culling
            if (m_ShadowDistanceCullingSq > 0.0f)
            {
                float distSq = glm::length2(trans.position - camPos);
                if (distSq > m_ShadowDistanceCullingSq)
                    continue;
            }

            // Light frustum culling
            if (m_ShadowFrustumCullingEnabled)
            {
                glm::mat4 modelMatrix = trans.GetWorldModelMatrix(scene.registry);
                glm::vec3 min = renderer.model->AABBmin;
                glm::vec3 max = renderer.model->AABBmax;
                
                glm::vec3 center = (min + max) * 0.5f;
                glm::vec3 extent = (max - min) * 0.5f;
                
                glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(center, 1.0f));
                
                glm::mat3 rot = glm::mat3(modelMatrix);
                glm::vec3 worldExtent = glm::vec3(
                    std::abs(rot[0][0]) * extent.x + std::abs(rot[1][0]) * extent.y + std::abs(rot[2][0]) * extent.z,
                    std::abs(rot[0][1]) * extent.x + std::abs(rot[1][1]) * extent.y + std::abs(rot[2][1]) * extent.z,
                    std::abs(rot[0][2]) * extent.x + std::abs(rot[1][2]) * extent.y + std::abs(rot[2][2]) * extent.z
                );
                
                glm::vec3 worldMin = worldCenter - worldExtent;
                glm::vec3 worldMax = worldCenter + worldExtent;

                if (!lightFrustum.IsBoxVisible(worldMin, worldMax))
                    continue;
            }

            shaderDir->setMat4("model", trans.GetWorldModelMatrix(scene.registry));
            
            // Handle animated models
            if (scene.registry.all_of<AnimationComponent>(entity))
            {
                auto &anim = scene.registry.get<AnimationComponent>(entity);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    for (int j = 0; j < transforms.size() && j < 100; ++j)
                        shaderDir->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                    shaderDir->setBool("hasAnimation", true);
                }
                else
                {
                    shaderDir->setBool("hasAnimation", false);
                }
            }
            else
            {
                shaderDir->setBool("hasAnimation", false);
            }
            
            renderer.model->Draw(*shaderDir);
        }
    }

    m_Shadow.UnbindFBO();

    // Point light shadows (unchanged, but update to use isCastShadow)
    auto pointView = scene.registry.view<PointLightComponent, TransformComponent>();
    
    if (!shaderPoint)
        return;
    
    std::vector<entt::entity> shadowCastingPointLights;
    for (auto entity : pointView)
    {
        auto& light = pointView.get<PointLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            shadowCastingPointLights.push_back(entity);
            if (shadowCastingPointLights.size() >= 4)
                break;
        }
    }
    
    if (shadowCastingPointLights.empty())
        return;

    int pIdx = 0;
    shaderPoint->use();
    for (auto entity : shadowCastingPointLights)
    {
        if (pIdx >= Shadow::MAX_POINT_LIGHTS_SHADOW)
            break;

        auto [light, trans] = pointView.get<PointLightComponent, TransformComponent>(entity);
        glm::vec3 lightPos = trans.position;

        float aspect = (float)m_Shadow.GetShadowPointWidth() / (float)m_Shadow.GetShadowPointHeight();
        float nearP = 1.0f;
        float farP = m_FarPlanePoint;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, nearP, farP);

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        for (int i = 0; i < 6; ++i)
            shaderPoint->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

        shaderPoint->setFloat("farPlane", farP);
        shaderPoint->setVec3("lightPos", lightPos);

        m_Shadow.BindFBO_Point(pIdx);
        glClear(GL_DEPTH_BUFFER_BIT);

        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto obj : view)
        {
            auto [tObj, rObj] = view.get<TransformComponent, MeshRendererComponent>(obj);
            if (rObj.model)
            {
                shaderPoint->setMat4("model", tObj.GetWorldModelMatrix(scene.registry));
                if (scene.registry.all_of<AnimationComponent>(obj))
                {
                    auto &anim = scene.registry.get<AnimationComponent>(obj);
                    if (anim.animator)
                    {
                        auto transforms = anim.animator->GetFinalBoneMatrices();
                        for (int j = 0; j < transforms.size() && j < 100; ++j)
                            shaderPoint->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                        shaderPoint->setBool("hasAnimation", true);
                    }
                    else
                    {
                        shaderPoint->setBool("hasAnimation", false);
                    }
                }
                else
                {
                    shaderPoint->setBool("hasAnimation", false);
                }
                rObj.model->Draw(*shaderPoint);
            }
        }

        pIdx++;
    }

    m_Shadow.UnbindFBO();
}

// GPU Light Structs (std430 alignment)
struct GPUDirLight {
    glm::vec3 direction; float pad0;
    glm::vec3 color; float intensity;
    glm::vec3 ambient; float pad1;
    glm::vec3 diffuse; float pad2;
    glm::vec3 specular; float pad3;
};

struct GPUPointLight {
    glm::vec3 position; float pad0;
    glm::vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    glm::vec3 ambient; float pad1;
    glm::vec3 diffuse; float pad2;
    glm::vec3 specular; float pad3;
};

struct GPUSpotLight {
    glm::vec3 position; float pad0;
    glm::vec3 direction; float pad1;
    glm::vec3 color; float intensity;
    float cutOff; float outerCutOff; float constant; float linear;
    float quadratic; float pad2; float pad3; float pad4;
    glm::vec3 ambient; float pad5;
    glm::vec3 diffuse; float pad6;
    glm::vec3 specular; float pad7;
};

void RenderSystem::UploadLightData(Scene &scene, Shader *shader)
{
    // Directional Lights
    std::vector<GPUDirLight> dirLights;
    auto dirView = scene.registry.view<DirectionalLightComponent>();
    
    // Find shadow-casting light first (for compatibility)
    entt::entity shadowCasterEntity = entt::null;
    for (auto entity : dirView)
    {
        auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (light.active && light.isCastShadow)
        {
             shadowCasterEntity = entity;
             glm::vec3 dir = light.direction;
             if (scene.registry.all_of<TransformComponent>(entity))
             {
                 auto& trans = scene.registry.get<TransformComponent>(entity);
                 dir = trans.rotation * glm::vec3(0, 0, -1);
             }
             
             dirLights.push_back({
                 dir, 0.0f,
                 light.color, light.intensity,
                 light.ambient, 0.0f,
                 light.diffuse, 0.0f,
                 light.specular, 0.0f
             });
             break; // First shadow-casting light
        }
    }

    // Add remaining lights
    for (auto entity : dirView)
    {
        if (entity == shadowCasterEntity) continue; // Skip already added shadow-caster

        auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.active) continue;

        glm::vec3 dir = light.direction;
        // Use Transform if available
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto& trans = scene.registry.get<TransformComponent>(entity);
            dir = trans.rotation * glm::vec3(0, 0, -1); // Forward vector -Z
        }

        dirLights.push_back({
            dir, 0.0f,
            light.color, light.intensity,
            light.ambient, 0.0f,
            light.diffuse, 0.0f,
            light.specular, 0.0f
        });
    }

    // Point Lights
    std::vector<GPUPointLight> pointLights;
    auto pointView = scene.registry.view<PointLightComponent>();
    for (auto entity : pointView)
    {
        auto& light = pointView.get<PointLightComponent>(entity);
        if (!light.active) continue;

        glm::vec3 pos = glm::vec3(0.0f);
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto& trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
        }

        pointLights.push_back({
            pos, 0.0f,
            light.color, light.intensity,
            light.constant, light.linear, light.quadratic, light.radius,
            light.ambient, 0.0f,
            light.diffuse, 0.0f,
            light.specular, 0.0f
        });
    }

    // Spot Lights
    std::vector<GPUSpotLight> spotLights;
    auto spotView = scene.registry.view<SpotLightComponent>();
    for (auto entity : spotView)
    {
        auto& light = spotView.get<SpotLightComponent>(entity);
        if (!light.active) continue;

        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 dir = light.direction;
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto& trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
            dir = trans.rotation * glm::vec3(0, 0, -1);
        }

        spotLights.push_back({
            pos, 0.0f,
            dir, 0.0f,
            light.color, light.intensity,
            light.cutOff, light.outerCutOff, light.constant, light.linear,
            light.quadratic, 0.0f, 0.0f, 0.0f,
            light.ambient, 0.0f,
            light.diffuse, 0.0f,
            light.specular, 0.0f
        });
    }

    // Upload to SSBOs
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_DirLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dirLights.size() * sizeof(GPUDirLight), dirLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_DirLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_PointLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointLights.size() * sizeof(GPUPointLight), pointLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_PointLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SpotLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spotLights.size() * sizeof(GPUSpotLight), spotLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_SpotLightSSBO);

    // Uniforms for Counts
    shader->setInt("numDirLights", (int)dirLights.size());
    shader->setInt("nrPointLights", (int)pointLights.size());
    shader->setInt("nrSpotLights", (int)spotLights.size());
    
    // For Shadow Rendering (legacy 0th light support for now or use the first one)
    if (!dirLights.empty())
    {
         // We might need to keep legacy uniforms if we don't update ALL logic
         // But for now, we assume shader reads from SSBO
    }
}

void RenderSystem::Render(Scene &scene)
{
    if (!m_Enabled) return;

    entt::entity camEntity = scene.GetActiveCamera();
    CameraComponent *cam = nullptr;
    TransformComponent *camTrans = nullptr;

    if (camEntity == entt::null)
        return;

    cam = &scene.registry.get<CameraComponent>(camEntity);
    camTrans = &scene.registry.get<TransformComponent>(camEntity);

    Frustum frustum;
    if (cam)
        frustum.Update(cam->projectionMatrix * cam->viewMatrix);

    m_RenderQueue.clear();

    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : view)
    {
        auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

        if (!renderer.model || !renderer.shader)
            continue;

        if (cam && m_FrustumCullingEnabled)
        {
            glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
            glm::vec3 min = renderer.model->AABBmin;
            glm::vec3 max = renderer.model->AABBmax;
            
            glm::vec3 center = (min + max) * 0.5f;
            glm::vec3 extent = (max - min) * 0.5f;
            
            glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(center, 1.0f));
            
            glm::mat3 rot = glm::mat3(modelMatrix);
            glm::vec3 worldExtent = glm::vec3(
                std::abs(rot[0][0]) * extent.x + std::abs(rot[1][0]) * extent.y + std::abs(rot[2][0]) * extent.z,
                std::abs(rot[0][1]) * extent.x + std::abs(rot[1][1]) * extent.y + std::abs(rot[2][1]) * extent.z,
                std::abs(rot[0][2]) * extent.x + std::abs(rot[1][2]) * extent.y + std::abs(rot[2][2]) * extent.z
            );
            
            glm::vec3 worldMin = worldCenter - worldExtent;
            glm::vec3 worldMax = worldCenter + worldExtent;

            if (!frustum.IsBoxVisible(worldMin, worldMax))
                continue;
        }

        m_RenderQueue.push_back({entity, &transform, &renderer});
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [](const RenderItem &lhs, const RenderItem &rhs) {
        if (lhs.renderer->shader != rhs.renderer->shader)
            return lhs.renderer->shader < rhs.renderer->shader;
        return lhs.renderer->model < rhs.renderer->model;
    });

    // Uniform string cache
    static std::vector<std::string> bonesUniforms;
    if (bonesUniforms.empty()) {
        for (int i = 0; i < 100; ++i) bonesUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");
    }
    static std::vector<std::string> shadowPointUniforms;
    if (shadowPointUniforms.empty()) {
        for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i) shadowPointUniforms.push_back("shadowMapPoint[" + std::to_string(i) + "]");
    }

    Shader *currentShader = nullptr;
    Model *currentModel = nullptr;
    std::vector<glm::mat4> instanceBatch;
    m_RenderedCount = 0;

    auto flushBatch = [&](Shader* shader, Model* model) {
        if (!instanceBatch.empty() && shader && model) {
            model->DrawInstanced(*shader, instanceBatch);
            m_RenderedCount += instanceBatch.size();
            instanceBatch.clear();
        }
    };

    for (const auto &item : m_RenderQueue)
    {
        entt::entity entity = item.entity;
        TransformComponent &transform = *item.transform;
        MeshRendererComponent &renderer = *item.renderer;
        
        // Setup shader if changed
        if (currentShader != renderer.shader)
        {
            flushBatch(currentShader, currentModel);
            currentShader = renderer.shader;
            currentModel = nullptr; // invalidate model to force setup
            currentShader->use();

            if (cam && camTrans)
            {
                currentShader->setMat4("projection", cam->projectionMatrix);
                currentShader->setMat4("view", cam->viewMatrix);
                currentShader->setVec3("viewPos", camTrans->position);

                // Upload all 4 light space matrices as array
                for (int i = 0; i < 4; ++i)
                {
                    std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
                    currentShader->setMat4(uniformName, m_LightSpaceMatrixDir[i]);
                }
                
                currentShader->setFloat("farPlanePoint", m_FarPlanePoint);
                currentShader->setBool("u_ReceiveShadow", m_EnableShadows && m_ShadowMode > 0);
                currentShader->setFloat("material.shininess", 32.0f);

                // Bind all 4 directional shadow maps
                for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                {
                    m_Shadow.BindTexture_Dir(i, 10 + i);
                    std::string uniformName = "shadowMapDir[" + std::to_string(i) + "]";
                    currentShader->setInt(uniformName, 10 + i);
                }

                // Bind point light shadow maps (starting from texture unit 14)
                for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
                {
                    m_Shadow.BindTexture_Point(i, 14 + i);
                    currentShader->setInt(shadowPointUniforms[i], 14 + i);
                }
            }
            UploadLightData(scene, currentShader);
        }

        bool isAnimated = scene.registry.all_of<AnimationComponent>(entity) && scene.registry.get<AnimationComponent>(entity).animator;

        if (isAnimated)
        {
            flushBatch(currentShader, currentModel);
            currentModel = nullptr;

            glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
            currentShader->setMat4("model", modelMatrix);
            currentShader->setVec4("tintColor", renderer.color);

            auto &anim = scene.registry.get<AnimationComponent>(entity);
            auto transforms = anim.animator->GetFinalBoneMatrices();
            for (int j = 0; j < transforms.size() && j < 100; ++j)
                currentShader->setMat4(bonesUniforms[j], transforms[j]);
            
             if (scene.registry.all_of<MaterialComponent>(entity))
            {
                auto &mat = scene.registry.get<MaterialComponent>(entity);
                if (mat.type == MaterialType::PBR)
                {
                    currentShader->setFloat("material.roughness", mat.roughness);
                    currentShader->setFloat("material.metallic", mat.metallic);
                    currentShader->setFloat("material.ao", mat.ao);
                    currentShader->setVec3("material.emission", mat.emission);
                }
                else
                {
                    currentShader->setFloat("material.shininess", mat.shininess);
                    currentShader->setVec3("material.specular", mat.specular);
                    currentShader->setVec3("material.ambient", mat.ambient);
                    currentShader->setVec3("material.emission", mat.emission);
                }
                currentShader->setFloat("material.opacity", mat.opacity);
                currentShader->setVec2("uvScale", mat.uvScale);
                currentShader->setVec2("uvOffset", mat.uvOffset);

                if (m_DebugNoTexture)
                {
                     if (m_WhiteTextureID == 0)
                     {
                         glGenTextures(1, &m_WhiteTextureID);
                         glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                         unsigned char white[] = {255, 255, 255, 255};
                         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                     }
                     glActiveTexture(GL_TEXTURE0);
                     glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                } 
            }
            else
            {
                 currentShader->setFloat("material.shininess", 32.0f); 
                 currentShader->setVec3("material.specular", glm::vec3(0.5f));
                 currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                 currentShader->setVec3("material.emission", glm::vec3(0.0f));
                 currentShader->setFloat("material.opacity", 1.0f);
                 currentShader->setVec2("uvScale", glm::vec2(1.0f));
                 currentShader->setVec2("uvOffset", glm::vec2(0.0f));
            }

            if (m_DebugNoTexture) currentShader->setBool("debug_noTexture", true);
            else currentShader->setBool("debug_noTexture", false);

            renderer.model->Draw(*currentShader);
            m_RenderedCount++;
        }
        else
        {
            //STATIC MESH RENDERING (Non-animated)
            if (m_DebugNoTexture) currentShader->setBool("debug_noTexture", true);
            else currentShader->setBool("debug_noTexture", false);

            if (!m_InstanceBatchingEnabled)
            {
                 // FORCE DIRECT DRAW (No Instancing)
                 currentShader->setMat4("model", transform.GetWorldModelMatrix(scene.registry));
                 currentShader->setVec4("tintColor", renderer.color);

                 if (scene.registry.all_of<MaterialComponent>(entity))
                 {
                     auto &mat = scene.registry.get<MaterialComponent>(entity);
                     if (mat.type == MaterialType::PBR)
                     {
                         currentShader->setFloat("material.roughness", mat.roughness);
                         currentShader->setFloat("material.metallic", mat.metallic);
                         currentShader->setFloat("material.ao", mat.ao);
                         currentShader->setVec3("material.emission", mat.emission);
                     }
                     else
                     {
                         currentShader->setFloat("material.shininess", mat.shininess);
                         currentShader->setVec3("material.specular", mat.specular);
                         currentShader->setVec3("material.ambient", mat.ambient);
                         currentShader->setVec3("material.emission", mat.emission);
                     }
                     currentShader->setFloat("material.opacity", mat.opacity);
                     currentShader->setVec2("uvScale", mat.uvScale);
                     currentShader->setVec2("uvOffset", mat.uvOffset);

                     if (m_DebugNoTexture)
                     {
                          if (m_WhiteTextureID == 0)
                          {
                              glGenTextures(1, &m_WhiteTextureID);
                              glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                              unsigned char white[] = {255, 255, 255, 255};
                              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                          }
                          glActiveTexture(GL_TEXTURE0);
                          glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                     }
                 }
                 else
                 {
                      currentShader->setFloat("material.shininess", 32.0f); 
                      currentShader->setVec3("material.specular", glm::vec3(0.5f));
                      currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                      currentShader->setVec3("material.emission", glm::vec3(0.0f));
                      currentShader->setFloat("material.opacity", 1.0f);
                      currentShader->setVec2("uvScale", glm::vec2(1.0f));
                      currentShader->setVec2("uvOffset", glm::vec2(0.0f));
                 }

                 renderer.model->Draw(*currentShader);
                 m_RenderedCount++;
            }
            else
            {
                 // INSTANCE BATCHING LOGIC
                 if (currentModel != renderer.model)
                 {
                     flushBatch(currentShader, currentModel);
                     currentModel = renderer.model;
                     
                     currentShader->setVec4("tintColor", renderer.color);
                     
                     if (scene.registry.all_of<MaterialComponent>(entity))
                     {
                         auto &mat = scene.registry.get<MaterialComponent>(entity);
                         if (mat.type == MaterialType::PBR)
                         {
                             currentShader->setFloat("material.roughness", mat.roughness);
                             currentShader->setFloat("material.metallic", mat.metallic);
                             currentShader->setFloat("material.ao", mat.ao);
                             currentShader->setVec3("material.emission", mat.emission);
                         }
                         else
                         {
                             currentShader->setFloat("material.shininess", mat.shininess);
                             currentShader->setVec3("material.specular", mat.specular);
                             currentShader->setVec3("material.ambient", mat.ambient);
                             currentShader->setVec3("material.emission", mat.emission);
                         }
                         currentShader->setFloat("material.opacity", mat.opacity);
                         currentShader->setVec2("uvScale", mat.uvScale);
                         currentShader->setVec2("uvOffset", mat.uvOffset);
                         
                         if (m_DebugNoTexture)
                         {
                              if (m_WhiteTextureID == 0)
                              {
                                  glGenTextures(1, &m_WhiteTextureID);
                                  glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                                  unsigned char white[] = {255, 255, 255, 255};
                                  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                                  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                              }
                              glActiveTexture(GL_TEXTURE0);
                              glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                         }
                     }
                     else
                     {
                          currentShader->setFloat("material.shininess", 32.0f); 
                          currentShader->setVec3("material.specular", glm::vec3(0.5f));
                          currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                          currentShader->setVec3("material.emission", glm::vec3(0.0f));
                          currentShader->setFloat("material.opacity", 1.0f);
                          currentShader->setVec2("uvScale", glm::vec2(1.0f));
                          currentShader->setVec2("uvOffset", glm::vec2(0.0f));
                     }
                     
                     if (m_DebugNoTexture) currentShader->setBool("debug_noTexture", true);
                     else currentShader->setBool("debug_noTexture", false);
                 }
                 
                 instanceBatch.push_back(transform.GetWorldModelMatrix(scene.registry));
            }
        }
    }
    flushBatch(currentShader, currentModel);
}

void RenderSystem::SetFaceCulling(bool enabled, int mode)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(mode);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void RenderSystem::SetDepthTest(bool enabled, int func)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(func);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}
