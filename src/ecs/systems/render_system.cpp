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

    res.LoadShader("shadow_depth", "resources/shaders/shadow_depth.vs", "resources/shaders/shadow_depth.fs");
    res.LoadShader("shadow_point", "resources/shaders/shadow_point.vs", "resources/shaders/shadow_point.fs", "resources/shaders/shadow_point.gs");

    m_Shadow.SetShaderDir(res.GetShader("shadow_depth"));
    m_Shadow.SetShaderPoint(res.GetShader("shadow_point"));
}

void RenderSystem::Shutdown()
{
    m_Shadow.Shutdown();
}

void RenderSystem::RenderShadows(Scene &scene)
{
    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();

    if (!shaderDir || !shaderPoint || !m_EnableShadows)
        return;

    auto dirLightView = scene.registry.view<DirectionalLightComponent>();

    glm::vec3 lightDir = glm::vec3(-1.0f);
    for (auto entity : dirLightView)
    {
        lightDir = dirLightView.get<DirectionalLightComponent>(entity).direction;
        break;
    }

    float orthoSize = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 150.0f);
    
    glm::vec3 lightPos = glm::vec3(0.0f) - glm::normalize(lightDir) * 75.0f; 
    
    glm::mat4 lightView = glm::lookAt(lightPos,
                                      glm::vec3(0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
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

    int pIdx = 0;
    shaderPoint->use();
    auto pView = scene.registry.view<PointLightComponent, TransformComponent>();

    for (auto entity : pView)
    {
        if (pIdx >= Shadow::MAX_POINT_LIGHTS_SHADOW)
            break;

        auto [light, trans] = pView.get<PointLightComponent, TransformComponent>(entity);
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
    for (auto entity : dirLightView)
    {
        auto &light = dirLightView.get<DirectionalLightComponent>(entity);
        shader->setVec3("dirLight.direction", light.direction);
        shader->setVec3("dirLight.ambient", light.ambient * light.intensity);
        shader->setVec3("dirLight.diffuse", light.diffuse * light.intensity);
        shader->setVec3("dirLight.specular", light.specular * light.intensity);
        shader->setVec3("dirLight.color", light.color * light.intensity);
        break;
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

    scene.registry.sort<MeshRendererComponent>([](const auto &lhs, const auto &rhs)
                                               { return lhs.shader < rhs.shader; });

    Shader *currentShader = nullptr;
    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    view.use<MeshRendererComponent>();

    m_RenderedCount = 0;

    for (auto entity : view)
    {
        auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

        if (!renderer.model || !renderer.shader)
            continue;

        glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);

        if (cam)
        {
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

        if (currentShader != renderer.shader)
        {
            currentShader = renderer.shader;
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
                    currentShader->setInt("shadowMapPoint[" + std::to_string(i) + "]", 11 + i);
                }
            }

            UploadLightData(scene, currentShader);
        }

        currentShader->setMat4("model", modelMatrix);
        currentShader->setVec4("tintColor", renderer.color);

        if (scene.registry.all_of<AnimationComponent>(entity))
        {
            auto &anim = scene.registry.get<AnimationComponent>(entity);
            if (anim.animator)
            {
                auto transforms = anim.animator->GetFinalBoneMatrices();
                for (int j = 0; j < transforms.size() && j < 100; ++j)
                    currentShader->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
            }
        }

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
            
            if (m_DebugNoTexture) currentShader->setBool("debug_noTexture", true);
            else currentShader->setBool("debug_noTexture", false);
        }

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
             
             currentShader->setBool("debug_noTexture", true);
        }
        else
        {
             currentShader->setBool("debug_noTexture", false);
        }

        renderer.model->Draw(*currentShader);
        
        m_RenderedCount++;
    }
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
