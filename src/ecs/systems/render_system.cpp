#include <ecs/system.h>
#include <graphic/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <resource/resource_manager.h>

void RenderSystem::InitShadows(ResourceManager &res)
{
    m_Shadow.Init();

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
    if (!m_EnableShadows)
        return;

    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();

    if (!shaderDir)
        return;

    auto dirLightView = scene.registry.view<DirectionalLightComponent>();
    DirectionalLightComponent* primaryLight = nullptr;
    
    for (auto entity : dirLightView)
    {
        auto& light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isPrimary && light.active)
        {
            primaryLight = &light;
            break;
        }
    }
    
    if (!primaryLight)
        return;

    glm::vec3 lightDir = glm::normalize(primaryLight->direction);
    glm::vec3 lightPos = -lightDir * 20.0f;

    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_LightSpaceMatrixDir = lightProjection * lightView;

    m_Shadow.BindFBO_Dir();
    glClear(GL_DEPTH_BUFFER_BIT);

    shaderDir->use();
    shaderDir->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir);

    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : view)
    {
        auto [trans, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);
        if (renderer.model)
        {
            shaderDir->setMat4("model", trans.GetWorldModelMatrix(scene.registry));
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

    m_Shadow.UnbindFBO();

    auto pointView = scene.registry.view<PointLightComponent, TransformComponent>();
    
    if (!shaderPoint)
        return;
    
    std::vector<entt::entity> primaryPointLights;
    for (auto entity : pointView)
    {
        auto& light = pointView.get<PointLightComponent>(entity);
        if (light.isPrimary && light.active)
        {
            primaryPointLights.push_back(entity);
            if (primaryPointLights.size() >= 4)
                break;
        }
    }
    
    if (primaryPointLights.empty())
        return;

    int pIdx = 0;
    shaderPoint->use();
    for (auto entity : primaryPointLights)
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

void RenderSystem::UploadLightData(Scene &scene, Shader *shader)
{
    auto dirLightView = scene.registry.view<DirectionalLightComponent>();
    int dirLightCount = 0;
    entt::entity primaryDirLightEntity = entt::null;
    entt::entity firstActiveDirLight = entt::null;
    
    for (auto entity : dirLightView)
    {
        auto& dirLight = dirLightView.get<DirectionalLightComponent>(entity);
        
        if (!dirLight.active)
            continue;
        
        if (firstActiveDirLight == entt::null)
            firstActiveDirLight = entity;
        
        if (dirLight.isPrimary)
            primaryDirLightEntity = entity;
        
        std::string idx = std::to_string(dirLightCount);
        shader->setVec3("dirLights[" + idx + "].direction", dirLight.direction);
        shader->setVec3("dirLights[" + idx + "].ambient", dirLight.ambient * dirLight.intensity);
        shader->setVec3("dirLights[" + idx + "].diffuse", dirLight.diffuse * dirLight.intensity);
        shader->setVec3("dirLights[" + idx + "].specular", dirLight.specular * dirLight.intensity);
        shader->setVec3("dirLights[" + idx + "].color", dirLight.color * dirLight.intensity);
        
        dirLightCount++;
        if (dirLightCount >= 4)
            break;
    }
    
    shader->setInt("numDirLights", dirLightCount);
    
    if (primaryDirLightEntity != entt::null)
    {
        auto& primaryDirLight = scene.registry.get<DirectionalLightComponent>(primaryDirLightEntity);
        shader->setVec3("dirLight.direction", primaryDirLight.direction);
        shader->setVec3("dirLight.ambient", primaryDirLight.ambient * primaryDirLight.intensity);
        shader->setVec3("dirLight.diffuse", primaryDirLight.diffuse * primaryDirLight.intensity);
        shader->setVec3("dirLight.specular", primaryDirLight.specular * primaryDirLight.intensity);
        shader->setVec3("dirLight.color", primaryDirLight.color * primaryDirLight.intensity);
    }
    else if (firstActiveDirLight != entt::null)
    {
        auto& firstDirLight = scene.registry.get<DirectionalLightComponent>(firstActiveDirLight);
        shader->setVec3("dirLight.direction", firstDirLight.direction);
        shader->setVec3("dirLight.ambient", firstDirLight.ambient * firstDirLight.intensity);
        shader->setVec3("dirLight.diffuse", firstDirLight.diffuse * firstDirLight.intensity);
        shader->setVec3("dirLight.specular", firstDirLight.specular * firstDirLight.intensity);
        shader->setVec3("dirLight.color", firstDirLight.color * firstDirLight.intensity);
    }


    static const std::string pointLightPos[4] = {"pointLights[0].position", "pointLights[1].position", "pointLights[2].position", "pointLights[3].position"};
    static const std::string pointLightAmb[4] = {"pointLights[0].ambient", "pointLights[1].ambient", "pointLights[2].ambient", "pointLights[3].ambient"};
    static const std::string pointLightDiff[4] = {"pointLights[0].diffuse", "pointLights[1].diffuse", "pointLights[2].diffuse", "pointLights[3].diffuse"};
    static const std::string pointLightSpec[4] = {"pointLights[0].specular", "pointLights[1].specular", "pointLights[2].specular", "pointLights[3].specular"};
    static const std::string pointLightColor[4] = {"pointLights[0].color", "pointLights[1].color", "pointLights[2].color", "pointLights[3].color"};
    static const std::string pointLightConst[4] = {"pointLights[0].constant", "pointLights[1].constant", "pointLights[2].constant", "pointLights[3].constant"};
    static const std::string pointLightLin[4] = {"pointLights[0].linear", "pointLights[1].linear", "pointLights[2].linear", "pointLights[3].linear"};
    static const std::string pointLightQuad[4] = {"pointLights[0].quadratic", "pointLights[1].quadratic", "pointLights[2].quadratic", "pointLights[3].quadratic"};

    int i = 0;
    auto pointLightView = scene.registry.view<PointLightComponent, TransformComponent>();
    for (auto entity : pointLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = pointLightView.get<PointLightComponent, TransformComponent>(entity);
        
        if (!light.active)
            continue;

        shader->setVec3(pointLightPos[i], trans.position);
        shader->setVec3(pointLightAmb[i], light.ambient * light.intensity);
        shader->setVec3(pointLightDiff[i], light.diffuse * light.intensity);
        shader->setVec3(pointLightSpec[i], light.specular * light.intensity);
        shader->setVec3(pointLightColor[i], light.color * light.intensity);
        shader->setFloat(pointLightConst[i], light.constant);
        shader->setFloat(pointLightLin[i], light.linear);
        shader->setFloat(pointLightQuad[i], light.quadratic);
        i++;
    }
    shader->setInt("nrPointLights", i);

    static const std::string spotLightPos[4] = {"spotLights[0].position", "spotLights[1].position", "spotLights[2].position", "spotLights[3].position"};
    static const std::string spotLightDir[4] = {"spotLights[0].direction", "spotLights[1].direction", "spotLights[2].direction", "spotLights[3].direction"};
    static const std::string spotLightCut[4] = {"spotLights[0].cutOff", "spotLights[1].cutOff", "spotLights[2].cutOff", "spotLights[3].cutOff"};
    static const std::string spotLightOut[4] = {"spotLights[0].outerCutOff", "spotLights[1].outerCutOff", "spotLights[2].outerCutOff", "spotLights[3].outerCutOff"};

    static const std::string spotLightAmb[4] = {"spotLights[0].ambient", "spotLights[1].ambient", "spotLights[2].ambient", "spotLights[3].ambient"};
    static const std::string spotLightDiff[4] = {"spotLights[0].diffuse", "spotLights[1].diffuse", "spotLights[2].diffuse", "spotLights[3].diffuse"};
    static const std::string spotLightSpec[4] = {"spotLights[0].specular", "spotLights[1].specular", "spotLights[2].specular", "spotLights[3].specular"};
    static const std::string spotLightConst[4] = {"spotLights[0].constant", "spotLights[1].constant", "spotLights[2].constant", "spotLights[3].constant"};
    static const std::string spotLightLin[4] = {"spotLights[0].linear", "spotLights[1].linear", "spotLights[2].linear", "spotLights[3].linear"};
    static const std::string spotLightQuad[4] = {"spotLights[0].quadratic", "spotLights[1].quadratic", "spotLights[2].quadratic", "spotLights[3].quadratic"};

    i = 0;
    auto spotLightView = scene.registry.view<SpotLightComponent, TransformComponent>();
    for (auto entity : spotLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = spotLightView.get<SpotLightComponent, TransformComponent>(entity);
        
        if (!light.active)
            continue;

        glm::vec3 direction = trans.rotation * glm::vec3(0, 0, -1);
        shader->setVec3(spotLightDir[i], direction);
        shader->setFloat(spotLightCut[i], light.cutOff);
        shader->setFloat(spotLightOut[i], light.outerCutOff);

        shader->setVec3(spotLightPos[i], trans.position);
        shader->setVec3(spotLightAmb[i], light.ambient * light.intensity);
        shader->setVec3(spotLightDiff[i], light.diffuse * light.intensity);
        shader->setVec3(spotLightSpec[i], light.specular * light.intensity);
        shader->setFloat(spotLightConst[i], light.constant);
        shader->setFloat(spotLightLin[i], light.linear);
        shader->setFloat(spotLightQuad[i], light.quadratic);

        i++;
    }
    shader->setInt("nrSpotLights", i);
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

        if (cam)
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

                currentShader->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir);
                currentShader->setFloat("farPlanePoint", m_FarPlanePoint);
                currentShader->setBool("u_ReceiveShadow", m_EnableShadows);
                currentShader->setFloat("material.shininess", 32.0f);

                m_Shadow.BindTexture_Dir(10);
                currentShader->setInt("shadowMapDir", 10);

                for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
                {
                    m_Shadow.BindTexture_Point(i, 11 + i);
                    currentShader->setInt(shadowPointUniforms[i], 11 + i);
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
