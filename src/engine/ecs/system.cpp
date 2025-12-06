#include <engine/ecs/system.h>

entt::entity Scene::createEntity()
{
    return registry.create();
}

entt::entity Scene::GetActiveCamera()
{
    auto view = registry.view<const CameraComponent>();
    for (auto entity : view)
    {
        const auto &cam = view.get<const CameraComponent>(entity);
        if (cam.isPrimary)
        {
            return entity;
        }
    }
    return entt::null;
}

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

    for (auto entity : view)
    {
        auto &anim = view.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            anim.animator->UpdateAnimation(dt);
        }
    }
}

void RenderSystem::Render(Scene &scene, Shader &shader)
{
    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto &cam = scene.registry.get<CameraComponent>(camEntity);
        auto &camTrans = scene.registry.get<TransformComponent>(camEntity);

        shader.use();
        shader.setMat4("projection", cam.projectionMatrix);
        shader.setMat4("view", cam.viewMatrix);
        shader.setVec3("viewPos", camTrans.position);
    }

    auto dirLightView = scene.registry.view<DirectionalLightComponent>();

    for (auto entity : dirLightView)
    {
        auto &light = dirLightView.get<DirectionalLightComponent>(entity);
        shader.setVec3("dirLight.direction", light.direction);
        shader.setVec3("dirLight.ambient", light.ambient * light.intensity);
        shader.setVec3("dirLight.diffuse", light.diffuse * light.intensity);
        shader.setVec3("dirLight.specular", light.specular * light.intensity);
        break;
    }

    int i = 0;
    auto spotLightView = scene.registry.view<SpotLightComponent, TransformComponent>();
    for (auto entity : spotLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = spotLightView.get<SpotLightComponent, TransformComponent>(entity);

        std::string number = std::to_string(i);
        shader.setVec3("spotLights[" + number + "].position", trans.position);
        shader.setVec3("spotLights[" + number + "].ambient", light.color * 0.1f * light.intensity);
        shader.setVec3("spotLights[" + number + "].diffuse", light.color * light.intensity);
        shader.setVec3("spotLights[" + number + "].specular", glm::vec3(1.0f) * light.intensity);
        shader.setFloat("spotLights[" + number + "].constant", light.constant);
        shader.setFloat("spotLights[" + number + "].linear", light.linear);
        shader.setFloat("spotLights[" + number + "].quadratic", light.quadratic);

        i++;
    }
    shader.setInt("nrSpotLights", i);

    i = 0;
    auto pointLightView = scene.registry.view<PointLightComponent, TransformComponent>();
    for (auto entity : pointLightView)
    {
        if (i >= 4)
            break;

        auto [light, trans] = pointLightView.get<PointLightComponent, TransformComponent>(entity);

        std::string number = std::to_string(i);
        shader.setVec3("pointLights[" + number + "].position", trans.position);
        shader.setVec3("pointLights[" + number + "].ambient", light.color * 0.1f * light.intensity);
        shader.setVec3("pointLights[" + number + "].diffuse", light.color * light.intensity);
        shader.setVec3("pointLights[" + number + "].specular", glm::vec3(1.0f) * light.intensity);
        shader.setFloat("pointLights[" + number + "].constant", light.constant);
        shader.setFloat("pointLights[" + number + "].linear", light.linear);
        shader.setFloat("pointLights[" + number + "].quadratic", light.quadratic);

        i++;
    }
    shader.setInt("nrPointLights", i);

    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();

    for (auto entity : view)
    {
        auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

        if (renderer.model)
        {
            glm::mat4 modelMatrix = transform.GetTransformMatrix();
            shader.setMat4("model", modelMatrix);

            if (scene.registry.all_of<AnimationComponent>(entity))
            {
                auto &anim = scene.registry.get<AnimationComponent>(entity);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    for (int j = 0; j < transforms.size(); ++j)
                    {
                        shader.setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                    }
                }
            }
            else
            {
                // Xử lý static mesh (có thể gửi ma trận identity hoặc dùng shader khác)
                // Ở đây giả sử shader yêu cầu thì phải gửi identity hoặc mảng rỗng
            }

            renderer.model->Draw(shader);
        }
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

void UISystem::Update(Scene &scene, float dt, const MouseManager &mouse)
{
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