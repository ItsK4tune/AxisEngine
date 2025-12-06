#include <engine/ecs/system.h>

size_t Scene::createEntity()
{
    transforms.emplace_back();
    renderers.emplace_back();
    rigidbodies.emplace_back();
    animators.emplace_back();
    cameras.emplace_back();

    cameras.back().isPrimary = false;

    return transforms.size() - 1;
}

EntityID Scene::GetActiveCameraID()
{
    for (size_t i = 0; i < cameras.size(); ++i)
    {
        if (cameras[i].isPrimary)
            return i;
    }
    return 0;
}

void PhysicsSystem::Update(Scene &scene)
{
    for (size_t i = 0; i < scene.rigidbodies.size(); ++i)
    {
        auto &rb = scene.rigidbodies[i];
        if (rb.body)
        {
            auto &transform = scene.transforms[i];
            btTransform trans;
            if (rb.body->getMotionState())
            {
                rb.body->getMotionState()->getWorldTransform(trans);
            }
            else
            {
                trans = rb.body->getWorldTransform();
            }

            transform.position = BulletGLMHelpers::convert(trans.getOrigin());
            transform.rotation = BulletGLMHelpers::convert(trans.getRotation());
        }
    }
}

void AnimationSystem::Update(Scene &scene, float dt)
{
    for (size_t i = 0; i < scene.animators.size(); ++i)
    {
        if (scene.animators[i].animator)
        {
            scene.animators[i].animator->UpdateAnimation(dt);
        }
    }
}

void RenderSystem::Render(Scene &scene, Shader &shader)
{
    EntityID camID = scene.GetActiveCameraID();
    auto& cam = scene.cameras[camID];
    
    shader.use();
    shader.setMat4("projection", cam.projectionMatrix);
    shader.setMat4("view", cam.viewMatrix);
    
    for (size_t i = 0; i < scene.renderers.size(); ++i)
    {
        if (scene.renderers[i].model)
        {
            shader.use();

            glm::mat4 modelMatrix = scene.transforms[i].GetTransformMatrix();
            shader.setMat4("model", modelMatrix);

            if (scene.animators[i].animator)
            {
                auto transforms = scene.animators[i].animator->GetFinalBoneMatrices();
                for (int j = 0; j < transforms.size(); ++j)
                {
                    shader.setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                }
            }
            else
            {
                // Reset bone matrices nếu không có animation (để tránh lỗi shader)
                // Hoặc dùng shader riêng cho static mesh
            }

            scene.renderers[i].model->Draw(shader);
        }
    }
}

void CameraSystem::Update(Scene &scene, float screenWidth, float screenHeight) 
{
    for (size_t i = 0; i < scene.cameras.size(); ++i) 
    {
        auto& cam = scene.cameras[i];
        if (!cam.isPrimary) continue;

        auto& transform = scene.transforms[i];

        cam.aspectRatio = screenWidth / screenHeight;

        cam.projectionMatrix = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);

        glm::vec3 front;
        front.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        front.y = sin(glm::radians(cam.pitch));
        front.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        cam.front = glm::normalize(front);

        cam.right = glm::normalize(glm::cross(cam.front, cam.worldUp));
        cam.up    = glm::normalize(glm::cross(cam.right, cam.front));

        cam.viewMatrix = glm::lookAt(transform.position, transform.position + cam.front, cam.up);
    }
}

void CameraControlSystem::Update(Scene &scene, float dt, const KeyboardManager& keyboard, const MouseManager& mouse)
{
    EntityID id = scene.GetActiveCameraID();
    if (id >= scene.cameras.size()) return;

    auto& cam = scene.cameras[id];
    auto& transform = scene.transforms[id];

    float sensitivity = 0.1f;
    cam.yaw += mouse.GetXOffset() * sensitivity;
    cam.pitch += mouse.GetYOffset() * sensitivity;

    if (cam.pitch > 89.0f) cam.pitch = 89.0f;
    if (cam.pitch < -89.0f) cam.pitch = -89.0f;
    
    float scroll = mouse.GetScrollY();
    if (scroll != 0.0f) {
        cam.fov -= scroll;
        if (cam.fov < 1.0f) cam.fov = 1.0f;
        if (cam.fov > 45.0f) cam.fov = 45.0f;
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