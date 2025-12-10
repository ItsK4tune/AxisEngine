#include <engine/ecs/system.h>
#include <engine/core/scriptable.h>

#include <execution>

void PhysicsSystem::Update(Scene &scene)
{
    auto view = scene.registry.view<RigidBodyComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto &rb = view.get<RigidBodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);

        if (rb.body)
        {
            btTransform trans;
            if (rb.body->getMotionState())
                rb.body->getMotionState()->getWorldTransform(trans);
            else
                trans = rb.body->getWorldTransform();

            transform.position = BulletGLMHelpers::convert(trans.getOrigin());
            transform.rotation = BulletGLMHelpers::convert(trans.getRotation());
        }
    }
}

void AnimationSystem::Update(Scene &scene, float dt)
{
    auto view = scene.registry.view<AnimationComponent>();

    std::vector<entt::entity> entities(view.begin(), view.end());

    std::for_each(std::execution::par, entities.begin(), entities.end(), [&scene, dt](entt::entity entity)
                  {
        auto &anim = scene.registry.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            anim.animator->UpdateAnimation(dt);
        } });
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
        break;
    }

    int i = 0;
    auto pointLightView = scene.registry.view<PointLightComponent, TransformComponent>();
    for (auto entity : pointLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = pointLightView.get<PointLightComponent, TransformComponent>(entity);
        std::string number = std::to_string(i);

        shader->setVec3("pointLights[" + number + "].position", trans.position);
        shader->setVec3("pointLights[" + number + "].ambient", light.color * 0.1f * light.intensity);
        shader->setVec3("pointLights[" + number + "].diffuse", light.color * light.intensity);
        shader->setVec3("pointLights[" + number + "].specular", glm::vec3(1.0f) * light.intensity);
        shader->setFloat("pointLights[" + number + "].constant", light.constant);
        shader->setFloat("pointLights[" + number + "].linear", light.linear);
        shader->setFloat("pointLights[" + number + "].quadratic", light.quadratic);
        i++;
    }
    shader->setInt("nrPointLights", i);

    i = 0;
    auto spotLightView = scene.registry.view<SpotLightComponent, TransformComponent>();
    for (auto entity : spotLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = spotLightView.get<SpotLightComponent, TransformComponent>(entity);

        std::string number = std::to_string(i);
        shader->setVec3("spotLights[" + number + "].position", trans.position);
        shader->setVec3("spotLights[" + number + "].ambient", light.color * 0.1f * light.intensity);
        shader->setVec3("spotLights[" + number + "].diffuse", light.color * light.intensity);
        shader->setVec3("spotLights[" + number + "].specular", glm::vec3(1.0f) * light.intensity);
        shader->setFloat("spotLights[" + number + "].constant", light.constant);
        shader->setFloat("spotLights[" + number + "].linear", light.linear);
        shader->setFloat("spotLights[" + number + "].quadratic", light.quadratic);

        i++;
    }
    shader->setInt("nrSpotLights", i);
}

void RenderSystem::Render(Scene &scene)
{
    entt::entity camEntity = scene.GetActiveCamera();
    CameraComponent *cam = nullptr;
    TransformComponent *camTrans = nullptr;

    if (camEntity != entt::null)
    {
        cam = &scene.registry.get<CameraComponent>(camEntity);
        camTrans = &scene.registry.get<TransformComponent>(camEntity);
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

        glm::mat4 modelMatrix = transform.GetTransformMatrix();
        currentShader->setMat4("model", modelMatrix);

        if (scene.registry.all_of<AnimationComponent>(entity))
        {
            auto &anim = scene.registry.get<AnimationComponent>(entity);
            if (anim.animator)
            {
                auto transforms = anim.animator->GetFinalBoneMatrices();
                for (int j = 0; j < transforms.size(); ++j)
                {
                    currentShader->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                }
            }
        }

        renderer.model->Draw(*currentShader);
    }
}

void CameraSystem::Update(Scene &scene, float screenWidth, float screenHeight)
{
    auto view = scene.registry.view<CameraComponent, const TransformComponent>();

    for (auto entity : view)
    {
        auto [cam, transform] = view.get<CameraComponent, const TransformComponent>(entity);

        if (!cam.isPrimary)
            continue;

        cam.aspectRatio = screenWidth / screenHeight;
        cam.projectionMatrix = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);

        glm::vec3 front;
        front.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        front.y = sin(glm::radians(cam.pitch));
        front.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        cam.front = glm::normalize(front);

        cam.right = glm::normalize(glm::cross(cam.front, cam.worldUp));
        cam.up = glm::normalize(glm::cross(cam.right, cam.front));

        cam.viewMatrix = glm::lookAt(transform.position, transform.position + cam.front, cam.up);
    }
}

void CameraControlSystem::Update(Scene &scene, float dt, const KeyboardManager &keyboard, const MouseManager &mouse)
{
    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &cam = scene.registry.get<CameraComponent>(camEntity);
    auto &transform = scene.registry.get<TransformComponent>(camEntity);

    float sensitivity = 0.1f;
    cam.yaw += mouse.GetXOffset() * sensitivity;
    cam.pitch += mouse.GetYOffset() * sensitivity;

    if (cam.pitch > 89.0f)
        cam.pitch = 89.0f;
    if (cam.pitch < -89.0f)
        cam.pitch = -89.0f;

    float scroll = mouse.GetScrollY();
    if (scroll != 0.0f)
    {
        cam.fov -= scroll;
        if (cam.fov < 1.0f)
            cam.fov = 1.0f;
        if (cam.fov > 45.0f)
            cam.fov = 45.0f;
    }

    float velocity = 2.5f * dt;
    if (keyboard.GetKey(GLFW_KEY_W))
        transform.position += cam.front * velocity;
    if (keyboard.GetKey(GLFW_KEY_S))
        transform.position -= cam.front * velocity;
    if (keyboard.GetKey(GLFW_KEY_A))
        transform.position -= cam.right * velocity;
    if (keyboard.GetKey(GLFW_KEY_D))
        transform.position += cam.right * velocity;
}

void UIRenderSystem::Render(Scene &scene, float screenWidth, float screenHeight)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    scene.registry.sort<UITransformComponent>([](const auto &lhs, const auto &rhs)
                                              { return lhs.zIndex < rhs.zIndex; });

    glm::mat4 projection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
    Shader *currentShader = nullptr;

    auto view = scene.registry.view<UITransformComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<UITransformComponent>(entity);

        if (auto *renderer = scene.registry.try_get<UIRendererComponent>(entity))
        {
            if (renderer->model && renderer->shader)
            {
                if (currentShader != renderer->shader)
                {
                    currentShader = renderer->shader;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("image", 0);
                }

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(transform.position, 0.0f));
                model = glm::scale(model, glm::vec3(transform.size, 1.0f));
                currentShader->setMat4("model", model);

                renderer->model->Draw(*currentShader, renderer->color);
            }
        }

        if (auto *textComp = scene.registry.try_get<UITextComponent>(entity))
        {
            if (textComp->model && textComp->shader && textComp->font)
            {
                if (currentShader != textComp->shader)
                {
                    currentShader = textComp->shader;
                    currentShader->use();
                    currentShader->setMat4("projection", projection);
                    currentShader->setInt("text", 0);
                }

                float x = transform.position.x;
                float y = transform.position.y;
                float scale = textComp->scale;

                for (char c : textComp->text)
                {
                    const Character &ch = textComp->font->GetCharacter(c);

                    float xpos = x + ch.Bearing.x * scale;
                    float ypos = y + (ch.Size.y - ch.Bearing.y) * scale;
                    float w = ch.Size.x * scale;
                    float h = ch.Size.y * scale;

                    std::vector<float> vertices = {
                        xpos, ypos - h, 0.0f, 0.0f,
                        xpos, ypos, 0.0f, 1.0f,
                        xpos + w, ypos, 1.0f, 1.0f,

                        xpos, ypos - h, 0.0f, 0.0f,
                        xpos + w, ypos, 1.0f, 1.0f,
                        xpos + w, ypos - h, 1.0f, 0.0f};

                    textComp->model->DrawDynamic(*currentShader, ch.TextureID, textComp->color, vertices);

                    x += (ch.Advance >> 6) * scale;
                }
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void UIInteractSystem::Update(Scene &scene, float dt, const MouseManager &mouse)
{
    if (mouse.GetCursorMode() == CursorMode::Hidden)
        return;

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();
    bool isMouseDown = mouse.IsLeftButtonPressed();

    auto view = scene.registry.view<UITransformComponent, UIInteractiveComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<UITransformComponent>(entity);
        auto &interact = view.get<UIInteractiveComponent>(entity);

        bool hit = (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
                    my >= transform.position.y && my <= transform.position.y + transform.size.y);

        if (hit)
        {
            if (!interact.isHovered)
            {
                interact.isHovered = true;
                if (interact.onHoverEnter)
                    interact.onHoverEnter(entity);
            }
        }
        else
        {
            if (interact.isHovered)
            {
                interact.isHovered = false;
                if (interact.onHoverExit)
                    interact.onHoverExit(entity);
            }
        }

        if (hit && isMouseDown)
        {
            if (!interact.isPressed)
            {
                interact.isPressed = true;
                if (interact.onClick)
                    interact.onClick(entity);
            }
        }
        else if (!isMouseDown)
        {
            interact.isPressed = false;
        }

        if (scene.registry.all_of<UIAnimationComponent>(entity))
        {
            auto &anim = scene.registry.get<UIAnimationComponent>(entity);
            auto &img = scene.registry.get_or_emplace<UIRendererComponent>(entity);

            if (interact.isHovered)
            {
                img.color = glm::mix(img.color, anim.hoverColor, dt * anim.speed);
                anim.targetScale = 1.2f;
            }
            else
            {
                img.color = glm::mix(img.color, anim.normalColor, dt * anim.speed);
                anim.targetScale = 1.0f;
            }

            anim.currentScale += (anim.targetScale - anim.currentScale) * dt * anim.speed;
        }
    }
}

void ScriptableSystem::Update(Scene& scene, float dt)
{
    auto view = scene.registry.view<ScriptComponent>();
    
    for(auto entity : view)
    {
        auto& nsc = view.get<ScriptComponent>(entity);

        if (!nsc.instance)
        {
            nsc.instance = nsc.InstantiateScript();
            nsc.instance->m_Entity = entity;
            nsc.instance->m_Scene = &scene;
            nsc.instance->OnCreate();
        }

        nsc.instance->OnUpdate(dt);
    }
}