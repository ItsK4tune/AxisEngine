#include <engine/ecs/system.h>
#include <engine/graphic/frustum.h>
#include <string>
#include <algorithm>
#include <vector>

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
        break;
    }

    // Optimized light data upload
    static const std::string pointLightPos[4] = { "pointLights[0].position", "pointLights[1].position", "pointLights[2].position", "pointLights[3].position" };
    static const std::string pointLightAmb[4] = { "pointLights[0].ambient", "pointLights[1].ambient", "pointLights[2].ambient", "pointLights[3].ambient" };
    static const std::string pointLightDiff[4] = { "pointLights[0].diffuse", "pointLights[1].diffuse", "pointLights[2].diffuse", "pointLights[3].diffuse" };
    static const std::string pointLightSpec[4] = { "pointLights[0].specular", "pointLights[1].specular", "pointLights[2].specular", "pointLights[3].specular" };
    static const std::string pointLightConst[4] = { "pointLights[0].constant", "pointLights[1].constant", "pointLights[2].constant", "pointLights[3].constant" };
    static const std::string pointLightLin[4] = { "pointLights[0].linear", "pointLights[1].linear", "pointLights[2].linear", "pointLights[3].linear" };
    static const std::string pointLightQuad[4] = { "pointLights[0].quadratic", "pointLights[1].quadratic", "pointLights[2].quadratic", "pointLights[3].quadratic" };

    int i = 0;
    auto pointLightView = scene.registry.view<PointLightComponent, TransformComponent>();
    for (auto entity : pointLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = pointLightView.get<PointLightComponent, TransformComponent>(entity);

        shader->setVec3(pointLightPos[i], trans.position);
        shader->setVec3(pointLightAmb[i], light.color * 0.1f * light.intensity);
        shader->setVec3(pointLightDiff[i], light.color * light.intensity);
        shader->setVec3(pointLightSpec[i], glm::vec3(1.0f) * light.intensity);
        shader->setFloat(pointLightConst[i], light.constant);
        shader->setFloat(pointLightLin[i], light.linear);
        shader->setFloat(pointLightQuad[i], light.quadratic);
        i++;
    }
    shader->setInt("nrPointLights", i);

    static const std::string spotLightPos[4] = { "spotLights[0].position", "spotLights[1].position", "spotLights[2].position", "spotLights[3].position" };
    static const std::string spotLightAmb[4] = { "spotLights[0].ambient", "spotLights[1].ambient", "spotLights[2].ambient", "spotLights[3].ambient" };
    static const std::string spotLightDiff[4] = { "spotLights[0].diffuse", "spotLights[1].diffuse", "spotLights[2].diffuse", "spotLights[3].diffuse" };
    static const std::string spotLightSpec[4] = { "spotLights[0].specular", "spotLights[1].specular", "spotLights[2].specular", "spotLights[3].specular" };
    static const std::string spotLightConst[4] = { "spotLights[0].constant", "spotLights[1].constant", "spotLights[2].constant", "spotLights[3].constant" };
    static const std::string spotLightLin[4] = { "spotLights[0].linear", "spotLights[1].linear", "spotLights[2].linear", "spotLights[3].linear" };
    static const std::string spotLightQuad[4] = { "spotLights[0].quadratic", "spotLights[1].quadratic", "spotLights[2].quadratic", "spotLights[3].quadratic" };

    i = 0;
    auto spotLightView = scene.registry.view<SpotLightComponent, TransformComponent>();
    for (auto entity : spotLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = spotLightView.get<SpotLightComponent, TransformComponent>(entity);

        shader->setVec3(spotLightPos[i], trans.position);
        shader->setVec3(spotLightAmb[i], light.color * 0.1f * light.intensity);
        shader->setVec3(spotLightDiff[i], light.color * light.intensity);
        shader->setVec3(spotLightSpec[i], glm::vec3(1.0f) * light.intensity);
        shader->setFloat(spotLightConst[i], light.constant);
        shader->setFloat(spotLightLin[i], light.linear);
        shader->setFloat(spotLightQuad[i], light.quadratic);

        i++;
    }
    shader->setInt("nrSpotLights", i);
}

void RenderSystem::Render(Scene &scene)
{
    entt::entity camEntity = scene.GetActiveCamera();
    CameraComponent *cam = nullptr;
    TransformComponent *camTrans = nullptr;

    if (camEntity == entt::null)
    {
        return;
    }

    cam = &scene.registry.get<CameraComponent>(camEntity);
    camTrans = &scene.registry.get<TransformComponent>(camEntity);

    // --- Frustum Culling Setup ---
    Frustum frustum;
    if (cam)
    {
        frustum.Update(cam->projectionMatrix * cam->viewMatrix);
    }

    scene.registry.sort<MeshRendererComponent>([](const auto &lhs, const auto &rhs)
                                               { return lhs.shader < rhs.shader; });

    Shader *currentShader = nullptr;
    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    view.use<MeshRendererComponent>();

    for (auto entity : view)
    {
        auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

        if (!renderer.model || !renderer.shader)
            continue;

        glm::mat4 modelMatrix = transform.GetTransformMatrix();

        // --- Frustum Culling Check ---
        if (cam)
        {
            glm::vec3 min = renderer.model->AABBmin;
            glm::vec3 max = renderer.model->AABBmax;

            // Transform 8 corners to world space
            std::vector<glm::vec3> corners = {
                {min.x, min.y, min.z}, {min.x, min.y, max.z},
                {min.x, max.y, min.z}, {min.x, max.y, max.z},
                {max.x, min.y, min.z}, {max.x, min.y, max.z},
                {max.x, max.y, min.z}, {max.x, max.y, max.z}
            };

            glm::vec3 minWorld(FLT_MAX);
            glm::vec3 maxWorld(-FLT_MAX);

            for (auto& p : corners)
            {
                glm::vec4 worldPos = modelMatrix * glm::vec4(p, 1.0f);
                minWorld.x = std::min(minWorld.x, worldPos.x);
                minWorld.y = std::min(minWorld.y, worldPos.y);
                minWorld.z = std::min(minWorld.z, worldPos.z);
                
                maxWorld.x = std::max(maxWorld.x, worldPos.x);
                maxWorld.y = std::max(maxWorld.y, worldPos.y);
                maxWorld.z = std::max(maxWorld.z, worldPos.z);
            }

            if (!frustum.IsBoxVisible(minWorld, maxWorld))
            {
                continue; // Skip rendering effectively
            }
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
                
                static std::vector<std::string> boneUniforms;
                if (boneUniforms.empty())
                {
                    boneUniforms.reserve(100);
                    for (int i = 0; i < 100; i++)
                        boneUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");
                }

                for (int j = 0; j < transforms.size() && j < 100; ++j)
                {
                    currentShader->setMat4(boneUniforms[j], transforms[j]);
                }
            }
        }

        renderer.model->Draw(*currentShader);
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
